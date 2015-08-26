/*
 *  Copyright (c) 2010,
 *  Gavriloaie Eugen-Andrei (shiretu@gmail.com)
 *
 *  This file is part of crtmpserver.
 *  crtmpserver is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  crtmpserver is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with crtmpserver.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "netio/netio.h"
#include "configuration/configfile.h"
#include "protocols/protocolmanager.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/defaultprotocolfactory.h"

//This is a structure that holds the state
//between config re-reads/re-runs

struct RunningStatus {
	// startup parameters
	Variant commandLine;

	//Configuration file
	ConfigFile *pConfigFile;

	//default protocol factory
	DefaultProtocolFactory * pProtocolFactory;

	//should we run again?
	bool run;

	//is this a daemon already?
	bool daemon;

	RunningStatus() {
		pConfigFile = NULL;
		pProtocolFactory = NULL;
		run = false;
		daemon = false;
	}
};

void QuitSignalHandler(void);
bool Initialize();
void Run();
void Cleanup();
void PrintHelp();
void PrintVersion();
void NormalizeCommandLine(string configFile);
bool ApplyUIDGID();
void WritePidFile(pid_t pid);

RunningStatus gRs;

#ifdef COMPILE_STATIC
BaseClientApplication *SpawnApplication(Variant configuration);
BaseProtocolFactory *SpawnFactory(Variant configuration);
#endif /* COMPILE_STATIC */

int main(int argc, const char **argv) {
	//1. Pick up the startup parameters and hold them inside the running status
	if (argc < 2) {
		fprintf(stdout, "Invalid command line. Use --help\n");
		return -1;
	}

	if (!Variant::DeserializeFromCmdLineArgs(argc, argv, gRs.commandLine)) {
		PrintHelp();
		return -1;
	}
	string configFile = argv[argc - 1];
	configFile = normalizePath(configFile, "");
	NormalizeCommandLine(configFile);

	if ((bool)gRs.commandLine["arguments"]["--help"]) {
		PrintHelp();
		return 0;
	}

	if ((bool)gRs.commandLine["arguments"]["--version"]) {
		PrintVersion();
		return 0;
	}

	if (configFile == "") {
		fprintf(stderr, "Configuration file not found: `%s`\n", argv[argc - 1]);
		return -1;
	}

	SRAND();
	InitNetworking();

	do {
		//2. Reset the run flag
		gRs.run = false;

		//3. Initialize the running status
		if (Initialize()) {
			Run();
		} else {
			gRs.run = false;
		}

		//5. Cleanup
		Cleanup();
	} while (gRs.run);

	//6. We are done
	return 0;
}

bool Initialize() {
	Logger::Init();

	if ((bool)gRs.commandLine["arguments"]["--use-implicit-console-appender"]) {
		Variant dummy;
		dummy[CONF_LOG_APPENDER_NAME] = "implicit console appender";
		dummy[CONF_LOG_APPENDER_TYPE] = CONF_LOG_APPENDER_TYPE_CONSOLE;
		dummy[CONF_LOG_APPENDER_COLORED] = (bool)true;
		dummy[CONF_LOG_APPENDER_LEVEL] = (uint32_t) 6;
		ConsoleLogLocation * pLogLocation = new ConsoleLogLocation(dummy);
		pLogLocation->SetLevel(_FINEST_);
		Logger::AddLogLocation(pLogLocation);
	}

	INFO("Reading configuration from %s", STR(gRs.commandLine["arguments"]["configFile"]));
#ifdef COMPILE_STATIC
	gRs.pConfigFile = new ConfigFile(SpawnApplication, SpawnFactory);
#else
	gRs.pConfigFile = new ConfigFile(NULL, NULL);
#endif
	string configFilePath = gRs.commandLine["arguments"]["configFile"];
	string fileName;
	string extension;
	splitFileName(configFilePath, fileName, extension);

	if (lowerCase(extension) == "xml") {
		if (!gRs.pConfigFile->LoadXmlFile(configFilePath,
				(bool)gRs.commandLine["arguments"]["--daemon"])) {
			FATAL("Unable to load file %s", STR(configFilePath));
			return false;
		}
	} else if (lowerCase(extension) == "lua") {
#ifdef HAS_LUA
		if (!gRs.pConfigFile->LoadLuaFile(configFilePath,
				(bool)gRs.commandLine["arguments"]["--daemon"])) {
			FATAL("Unable to load file %s", STR(configFilePath));
			return false;
		}
#else /* HAS_LUA */
		fprintf(stdout, "Lua is not supported by the current build of the server\n");
		ASSERT("Lua is not supported by the current build of the server");
		return false;
#endif /* HAS_LUA */
	} else {
		FATAL("Invalid file format: %s", STR(configFilePath));
		return false;
	}

	uint32_t currentFdCount = 0;
	uint32_t maxFdCount = 0;
	if ((!setMaxFdCount(currentFdCount, maxFdCount))
			|| (!enableCoreDumps())) {
		WARN("Unable to apply file descriptors count limits and activate core dumps");
	}

#ifndef WIN32
	if (gRs.pConfigFile->IsDaemon()) {
		if (!gRs.daemon) {
			INFO("Daemonize...");
			setFdCloseOnExec(STDIN_FILENO);
			setFdCloseOnExec(STDOUT_FILENO);
			setFdCloseOnExec(STDERR_FILENO);
			pid_t pid = fork();
			if (pid < 0) {
				FATAL("Unable to start as daemon. fork() failed");
				return false;
			}

			if (pid > 0) {
				if (gRs.commandLine["arguments"].HasKey("--pid"))
					WritePidFile(pid);
				return false;
			}

			FINEST("Create a new SID for the daemon");
			pid_t sid = setsid();
			if (sid < 0) {
				FATAL("Unable to start as daemon. setsid() failed");
				return false;
			}

			gRs.daemon = true;

			Logger::SignalFork();
		}
	}
#endif /* WIN32 */

	INFO("Configure logger");
	if (!gRs.pConfigFile->ConfigLogAppenders()) {
		FATAL("Unable to configure log appenders");
		return false;
	}

	INFO("%s", STR(Version::GetBanner()));

	INFO("OS files descriptors count limits: %"PRIu32"/%"PRIu32,
			currentFdCount, maxFdCount);

	INFO("Initialize I/O handlers manager: %s", NETWORK_REACTOR);
	IOHandlerManager::Initialize();

	INFO("Configure modules");
	if (!gRs.pConfigFile->ConfigModules()) {
		FATAL("Unable to configure modules");
		return false;
	}

	INFO("Plug in the default protocol factory");
	gRs.pProtocolFactory = new DefaultProtocolFactory();
	if (!ProtocolFactoryManager::RegisterProtocolFactory(gRs.pProtocolFactory)) {
		FATAL("Unable to register default protocols factory");
		return false;
	}

	INFO("Configure factories");
	if (!gRs.pConfigFile->ConfigFactories()) {
		FATAL("Unable to configure factories");
		return false;
	}

	INFO("Configure acceptors");
	if (!gRs.pConfigFile->ConfigAcceptors()) {
		FATAL("Unable to configure acceptors");
		return false;
	}

	INFO("Configure instances");
	if (!gRs.pConfigFile->ConfigInstances()) {
		FATAL("Unable to configure instances");
		return false;
	}

	INFO("Start I/O handlers manager: %s", NETWORK_REACTOR);
	IOHandlerManager::Start();

	INFO("Configure applications");
	if (!gRs.pConfigFile->ConfigApplications()) {
		FATAL("Unable to configure applications");
		return false;
	}

	INFO("Install the quit signal");
	installQuitSignal(QuitSignalHandler);

	return true;
}

void Run() {
	if (!ApplyUIDGID()) {
		FATAL("Unable to apply user id");
		exit(-1);
	}
	INFO("\n%s", STR(gRs.pConfigFile->GetServicesInfo()));
	INFO("GO! GO! GO! (%"PRIu32")", (uint32_t) GetPid());
	while (IOHandlerManager::Pulse()) {
		IOHandlerManager::DeleteDeadHandlers();
		ProtocolManager::CleanupDeadProtocols();
	}
}

void Cleanup() {
	WARN("Shutting down protocols manager");
	ProtocolManager::Shutdown();
	ProtocolManager::CleanupDeadProtocols();

	WARN("Shutting down I/O handlers manager");
	IOHandlerManager::ShutdownIOHandlers();
	IOHandlerManager::DeleteDeadHandlers();
	IOHandlerManager::Shutdown();

	WARN("Unregister and delete default protocol handler");
	ProtocolFactoryManager::UnRegisterProtocolFactory(gRs.pProtocolFactory);
	delete gRs.pProtocolFactory;
	gRs.pProtocolFactory = NULL;

	WARN("Shutting down applications");
	ClientApplicationManager::Shutdown();

	WARN("Delete the configuration");
	delete gRs.pConfigFile;
	gRs.pConfigFile = NULL;

	WARN("Doing final OpenSSL cleanup");
	CleanupSSL();

	WARN("Shutting down the logger leaving you in the dark. Bye bye... :(");
	Logger::Free(true);
}

void QuitSignalHandler(void) {
	IOHandlerManager::SignalShutdown();
}

void PrintHelp() {
	fprintf(stdout, "Usage: \n%s [OPTIONS] [config_file_path]\n\n", STR(gRs.commandLine["program"]));
	fprintf(stdout, "OPTIONS:\n");
	fprintf(stdout, "    --help\n");
	fprintf(stdout, "      Prints this help and exit\n\n");
	fprintf(stdout, "    --version\n");
	fprintf(stdout, "      Prints the version and exit.\n\n");
	fprintf(stdout, "    --use-implicit-console-appender\n");
	fprintf(stdout, "      Adds a console log appender.\n");
	fprintf(stdout, "      Particularly useful when the server starts and then stops immediately.\n");
	fprintf(stdout, "      Allows you to see if something is wrong with the config file\n\n");
	fprintf(stdout, "    --daemon\n");
	fprintf(stdout, "      Overrides the daemon setting inside the config file and forces\n");
	fprintf(stdout, "      the server to start in daemon mode.\n\n");
	fprintf(stdout, "    --uid=<uid>\n");
	fprintf(stdout, "      Run the process with the specified user id\n\n");
	fprintf(stdout, "    --gid=<gid>\n");
	fprintf(stdout, "      Run the process with the specified group id\n\n");
	fprintf(stdout, "    --pid=<pid_file>\n");
	fprintf(stdout, "      Create PID file.\n");
	fprintf(stdout, "      Works only if --daemon option is specified\n\n");
}

void PrintVersion() {
	fprintf(stdout, "%s\n", STR(Version::GetBanner()));
	if (Version::GetBuilderOSUname() != "")
		fprintf(stdout, "Compiled on machine: `%s`\n", STR(Version::GetBuilderOSUname()));
}

void NormalizeCommandLine(string configFile) {
	gRs.commandLine["arguments"]["configFile"] = configFile;
	gRs.commandLine["arguments"].RemoveKey(configFile);
	bool tmp = (gRs.commandLine["--help"] != V_NULL);
	gRs.commandLine["--help"] = (bool)tmp;
	tmp = (gRs.commandLine["--version"] != V_NULL);
	gRs.commandLine["--version"] = (bool)tmp;
	tmp = (gRs.commandLine["arguments"]["--use-implicit-console-appender"] != V_NULL);
	gRs.commandLine["arguments"]["--use-implicit-console-appender"] = (bool)tmp;
	tmp = (gRs.commandLine["arguments"]["--daemon"] != V_NULL);
	gRs.commandLine["arguments"]["--daemon"] = (bool)tmp;
	if (gRs.commandLine["arguments"].HasKey("--uid")) {
		gRs.commandLine["arguments"]["--uid"] = (uint32_t) atoi(STR(gRs.commandLine["arguments"]["--uid"]));
	} else {
		gRs.commandLine["arguments"]["--uid"] = (uint32_t) 0;
	}
	if (gRs.commandLine["arguments"].HasKey("--gid")) {
		gRs.commandLine["arguments"]["--gid"] = (uint32_t) atoi(STR(gRs.commandLine["arguments"]["--gid"]));
	} else {
		gRs.commandLine["arguments"]["--gid"] = (uint32_t) 0;
	}
}

bool ApplyUIDGID() {
#ifndef WIN32
	if ((uint32_t) gRs.commandLine["arguments"]["--gid"] != 0) {
		if (setgid((uid_t) gRs.commandLine["arguments"]["--gid"]) != 0) {
			FATAL("Unable to set GID");
			return false;
		}
	}
	if ((uint32_t) gRs.commandLine["arguments"]["--uid"] != 0) {
		if (setuid((uid_t) gRs.commandLine["arguments"]["--uid"]) != 0) {
			FATAL("Unable to set UID");
			return false;
		}
	}
#endif
	return true;
}

void WritePidFile(pid_t pid) {
	/*!
	 * rewrite PID file if it already exists
	 */
	string pidFile = gRs.commandLine["arguments"]["--pid"];
	struct stat sb;
	if (stat(STR(pidFile), &sb) == 0) {
		WARN("pid file %s already exists\n", STR(pidFile));
	} else {
		int err = errno;
		if (err != ENOENT) {
			WARN("stat: (%d) %s", err, strerror(err));
			return;
		}
	}

	File f;
	if (!f.Initialize(STR(pidFile), FILE_OPEN_MODE_TRUNCATE)) {
		WARN("Unable to open PID file %s", STR(pidFile));
		return;
	}

	string content = format("%"PRIz"d", pid);
	if (!f.WriteString(content)) {
		WARN("Unable to write PID to file %s", STR(pidFile));
		return;
	}
	f.Close();
}

#ifdef COMPILE_STATIC
#ifdef HAS_APP_ADMIN
extern "C" BaseClientApplication *GetApplication_admin(Variant configuration);
#endif
#ifdef HAS_APP_APPLESTREAMINGCLIENT
extern "C" BaseClientApplication *GetApplication_applestreamingclient(Variant configuration);
#endif
#ifdef HAS_APP_APPSELECTOR
extern "C" BaseClientApplication *GetApplication_appselector(Variant configuration);
#endif
#ifdef HAS_APP_FLVPLAYBACK
extern "C" BaseClientApplication *GetApplication_flvplayback(Variant configuration);
#endif
#ifdef HAS_APP_PROXYPUBLISH
extern "C" BaseClientApplication *GetApplication_proxypublish(Variant configuration);
#endif
#ifdef HAS_APP_SAMPLEFACTORY
extern "C" BaseClientApplication *GetApplication_samplefactory(Variant configuration);
extern "C" BaseProtocolFactory *GetFactory_samplefactory(Variant configuration);
#endif
#ifdef HAS_APP_STRESSTEST
extern "C" BaseClientApplication *GetApplication_stresstest(Variant configuration);
#endif
#ifdef HAS_APP_VPTESTS
extern "C" BaseClientApplication *GetApplication_vptests(Variant configuration);
#endif
#ifdef HAS_APP_VMAPP
extern "C" BaseClientApplication *GetApplication_vmapp(Variant configuration);
#endif

BaseClientApplication *SpawnApplication(Variant configuration) {
	if (false) {

	}
#ifdef HAS_APP_ADMIN
	else if (configuration[CONF_APPLICATION_NAME] == "admin") {
		return GetApplication_admin(configuration);
	}
#endif
#ifdef HAS_APP_APPLESTREAMINGCLIENT
	else if (configuration[CONF_APPLICATION_NAME] == "applestreamingclient") {
		return GetApplication_applestreamingclient(configuration);
	}
#endif
#ifdef HAS_APP_APPSELECTOR
	else if (configuration[CONF_APPLICATION_NAME] == "appselector") {
		return GetApplication_appselector(configuration);
	}
#endif
#ifdef HAS_APP_FLVPLAYBACK
	else if (configuration[CONF_APPLICATION_NAME] == "flvplayback") {
		return GetApplication_flvplayback(configuration);
	}
#endif
#ifdef HAS_APP_PROXYPUBLISH
	else if (configuration[CONF_APPLICATION_NAME] == "proxypublish") {
		return GetApplication_proxypublish(configuration);
	}
#endif
#ifdef HAS_APP_SAMPLEFACTORY
	else if (configuration[CONF_APPLICATION_NAME] == "samplefactory") {
		return GetApplication_samplefactory(configuration);
	}
#endif
#ifdef HAS_APP_STRESSTEST
	else if (configuration[CONF_APPLICATION_NAME] == "stresstest") {
		return GetApplication_stresstest(configuration);
	}
#endif
#ifdef HAS_APP_VPTESTS
	else if (configuration[CONF_APPLICATION_NAME] == "vptests") {
		return GetApplication_vptests(configuration);
	}
#endif
#ifdef HAS_APP_VMAPP
	else if (configuration[CONF_APPLICATION_NAME] == "vmapp") {
		return GetApplication_vmapp(configuration);
	}
#endif
	else {
		return NULL;
	}
}

BaseProtocolFactory *SpawnFactory(Variant configuration) {
	if (false) {

	}
#ifdef HAS_APP_SAMPLEFACTORY
	else if (configuration[CONF_APPLICATION_NAME] == "samplefactory") {
		return GetFactory_samplefactory(configuration);
	}
#endif
	else {
		return NULL;
	}
}
#endif


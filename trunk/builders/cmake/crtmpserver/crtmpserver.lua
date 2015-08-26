-- Start of the configuration. This is the only node in the config file. 
-- The rest of them are sub-nodes
configuration=
{
	-- if true, the server will run as a daemon.
	-- NOTE: all console appenders will be ignored if this is a daemon
	daemon=false,
	-- the OS's path separator. Used in composing paths
	pathSeparator="/",

	-- this is the place where all the logging facilities are setted up
	-- you can add/remove any number of locations

	logAppenders=
	{
		{
			-- name of the appender. Not too important, but is mandatory
			name="console appender",
			-- type of the appender. We can have the following values:
			-- console, coloredConsole and file
			-- NOTE: console appenders will be ignored if we run the server
			-- as a daemon
			type="coloredConsole",
			-- the level of logging. 6 is the FINEST message, 0 is FATAL message.
			-- The appender will "catch" all the messages below or equal to this level
			-- bigger the level, more messages are recorded
			level=6
		},
		{
			name="file appender",
			type="file",
			level=6,
			-- the file where the log messages are going to land
			fileName="/tmp/crtmpserver",
			--newLineCharacters="\r\n",
			fileHistorySize=10,
			fileLength=1024*256,
			singleLine=true
		}
	},
	
	-- this node holds all the RTMP applications
	applications=
	{
		-- this is the root directory of all applications
		-- usually this is relative to the binary execuable
		rootDirectory="applications",
		
		
		--this is where the applications array starts
		{
			-- The name of the application. It is mandatory and must be unique 
			name="appselector",
			-- Short description of the application. Optional
			description="Application for selecting the rest of the applications",
			
			-- The type of the application. Possible values are:
			-- dynamiclinklibrary - the application is a shared library
			protocol="dynamiclinklibrary",
			-- the complete path to the library. This is optional. If not provided, 
			-- the server will try to load the library from here
			-- <rootDirectory>/<name>/lib<name>.{so|dll|dylib}
			-- library="/some/path/to/some/shared/library.so"
			
			-- Tells the server to validate the clien's handshake before going further. 
			-- It is optional, defaulted to true
			validateHandshake=false,
			-- this is the folder from where the current application gets it's content.
			-- It is optional. If not specified, it will be defaulted to:
			-- <rootDirectory>/<name>/mediaFolder
			-- mediaFolder="/some/directory/where/media/files/are/stored"
			-- the application will also be known by that names. It is optional
			--aliases=
			--{
			--	"simpleLive",
			--	"vod",
			--	"live",
			--},
			-- This flag designates the default application. The default application
			-- is responsable of analyzing the "connect" request and distribute 
			-- the future connection to the correct application.
			default=true,
			acceptors = 
			{
				{
					ip="0.0.0.0",
					port=1935,
					protocol="inboundRtmp"
				},
				{
					ip="0.0.0.0",
					port=8081,
					protocol="inboundRtmps",
					sslKey="server.key",
					sslCert="server.crt"
				},
				{
					ip="0.0.0.0",
					port=8080,
					protocol="inboundRtmpt"
                },
			}
		},
		{
			description="FLV Playback Sample",
			name="flvplayback",
			protocol="dynamiclinklibrary",
			aliases=
			{
				"simpleLive",
				"vod",
				"live",
				"WeeklyQuest",
				"SOSample",
				"oflaDemo",
			},
			acceptors = 
			{
				{
					ip="0.0.0.0",
					port=6666,
					protocol="inboundLiveFlv",
					waitForMetadata=true,
				},
				{
					ip="0.0.0.0",
					port=9999,
					protocol="inboundTcpTs"
				},
				--[[{
					ip="0.0.0.0",
					port=7654,
					protocol="inboundRawHttpStream",
					crossDomainFile="/tmp/crossdomain.xml"
				},]]--
				{
					ip="0.0.0.0",
					port=5544,
					protocol="inboundRtsp"
				},
			},
			externalStreams = 
			{
				--[[
				{
					uri="rtsp://fms20.mediadirect.ro/live2/realitatea/realitatea",
					localStreamName="rtsp_test",
					forceTcp=true
				},
				{
					uri="rtmp://edge01.fms.dutchview.nl/botr/bunny",
					localStreamName="rtmp_test",
					swfUrl="http://www.example.com/example.swf",
					pageUrl="http://www.example.com/",
					tcUrl="rtmp://edge01.fms.dutchview.nl/botr/bunny", --this one is usually required and should have the same value as the uri
					emulateUserAgent="MAC 10,1,82,76",
				}]]--
			},
			validateHandshake=false,
			--enableCheckBandwidth=true,
			--[[authentication=
			{
				rtmp={
					type="adobe",
					encoderAgents=
					{
						"FMLE/3.0 (compatible; FMSc/1.0)",
						"My user agent",
					},
					usersFile="users.lua"
				},
				rtsp={
					usersFile="users.lua"
				}
			}, --]]
			mediaStorage = {
				namedStorage1={
					--this storage contains all properties with their
					--default values. The only mandatory property is
					--mediaFolder
					description="Some storage",
					mediaFolder="/Volumes/Storage/media/",
					metaFolder="/tmp/metadata",
					enableStats=false,
					clientSideBuffer=15,
					keyframeSeek=false,
					seekGranularity=0.1,
				},
				namedStorage2={
					mediaFolder="/Volumes/Storage/media/mp4",
					metaFolder="/tmp/metadata",
					seekGranularity=0.2,
					enableStats=true,
				},
				namedStorage3={
					mediaFolder="/Volumes/Storage/media/flv",
					metaFolder="/tmp/metadata",
				},
				{
					--this one doesn't have a name
					mediaFolder="/Volumes/Storage/media/mp3",
				}
			},
		},
		{
			name="samplefactory",
			description="asdsadasdsa",
			protocol="dynamiclinklibrary",
			aliases=
			{
				"httpOutboundTest"
			},
			acceptors = 
			{
				{
					ip="0.0.0.0",
					port=8989,
					protocol="httpEchoProtocol"
				},
				{
					ip="0.0.0.0",
					port=8988,
					protocol="echoProtocol"
				}
			},
			validateHandshake=false,
			--default=true,
		},
		{
			name="vptests",
			description="Variant protocol tests",
			protocol="dynamiclinklibrary",
			aliases=
			{
				"vptests_alias1",
				"vptests_alias2",
				"vptests_alias3",
			},
			acceptors = 
			{
				{
					ip="0.0.0.0",
					port=1111,
					protocol="inboundHttpXmlVariant"
				}
			},
			validateHandshake=false,
			--default=true,
		},
		{
			name="admin",
			description="Application for administering",
			protocol="dynamiclinklibrary",
			aliases=
			{
				"admin_alias1",
				"admin_alias2",
				"admin_alias3",
			},
			acceptors = 
			{
				{
					ip="0.0.0.0",
					port=1112,
					protocol="inboundJsonCli",
					useLengthPadding=true
				},
			},
			validateHandshake=false,
			--default=true,
		},
		{
			name="proxypublish",
			description="Application for forwarding streams to another RTMP server",
			protocol="dynamiclinklibrary",
			acceptors =
			{
				{	
					ip="0.0.0.0",
					port=6665,
					protocol="inboundLiveFlv"
				},
			},
			abortOnConnectError=true,
			targetServers = 
			{
				--[[{
					targetUri="rtmp://x.xxxxxxx.fme.ustream.tv/ustreamVideo/xxxxxxx",
					targetStreamName="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
					localStreamName="stream1",
					emulateUserAgent="FMLE/3.0 (compatible; FMSc/1.0 http://www.rtmpd.com)"
				}]]--,
				--[[{
					targetUri="rtmp://gigi:spaima@localhost/vod",
					targetStreamType="live", -- (live, record or append)
					emulateUserAgent="My user agent",
					localStreamName="stream1",
					keepAlive=true
				},]]--
			},
			externalStreams = 
			{
				--[[{
					uri="rtsp://fms20.mediadirect.ro/live2/realitatea/realitatea",
					localStreamName="stream1",
					forceTcp=true,
					keepAlive=true
				},]]--
			},
			validateHandshake=false,
			--default=true,
		},
		{
			name="stresstest",
			description="Application for stressing a streaming server",
			protocol="dynamiclinklibrary",
			targetServer="localhost",
			targetApp="vod",
			active=false,
			--[[streams = 
			{
				"lg00","lg01","lg02","lg03","lg04","lg05","lg06","lg07","lg08",
				"lg09","lg10","lg11","lg12","lg13","lg14","lg15","lg16","lg17",
				"lg18","lg19","lg20","lg21","lg22","lg23","lg24","lg25","lg26",
				"lg27","lg28","lg29","lg30","lg31","lg32","lg33","lg34","lg35",
				"lg36","lg37","lg38","lg39","lg40","lg41","lg42","lg43","lg44",
				"lg45","lg46","lg47","lg48","lg49"
			},]]--
			streams = 
			{
				"mp4:lg.mp4"
			},
			numberOfConnections=10,
			randomAccessStreams=false
		},
		--[[{
			name="vmapp",
			description="An application demonstrating the use of virtual machines",
			protocol="dynamiclinklibrary",
			vmType="lua",
			script="flvplayback.lua",
			aliases=
			{
				"flvplayback1",
				"vod1"
			},
			acceptors=
			{
				{
					ip="0.0.0.0",
					port=6544,
					protocol="inboundTcpTs"
				}
			}
		},]]--
		--#INSERTION_MARKER# DO NOT REMOVE THIS. USED BY appscaffold SCRIPT.
	}
}


/* Initialisation Class (not publicly disclosed to RsIFace) */

class RsInit
{
	public:
		/* Commandline/Directory options */

		static const char *RsConfigDirectory() ;

		static bool	setStartMinimised() ;
		static int 	InitRetroShare(int argcIgnored, char **argvIgnored) ;
		static int 	LoadCertificates(bool autoLoginNT) ;
		static bool	ValidateCertificate(std::string &userName) ;
		static bool	ValidateTrustedUser(std::string fname, std::string &userName) ;
		static bool	LoadPassword(std::string passwd) ;
		static bool	RsGenerateCertificate(std::string name, std::string org, std::string loc, std::string country, std::string passwd, std::string &errString);
		static void	load_check_basedir() ;
		static int	create_configinit() ;
		static bool RsStoreAutoLogin() ;
		static bool RsTryAutoLogin() ;
		static bool RsClearAutoLogin(std::string basedir) ;
		static void	InitRsConfig() ;

		static std::string getHomePath() ;

		/* Key Parameters that must be set before
		 * RetroShare will start up:
		 */
		static std::string load_cert;
		static std::string load_key;
		static std::string passwd;

		static bool havePasswd; 		/* for Commandline password */
		static bool autoLogin;  		/* autoLogin allowed */
		static bool startMinimised; /* Icon or Full Window */

		/* Win/Unix Differences */
		static char dirSeperator;

		/* Directories */
		static std::string basedir;
		static std::string homePath;

		/* Listening Port */
		static bool forceExtPort;
		static bool forceLocalAddr;
		static unsigned short port;
		static char inet[256];

		/* Logging */
		static bool haveLogFile;
		static bool outStderr;
		static bool haveDebugLevel;
		static int  debugLevel;
		static char logfname[1024];

		static bool firsttime_run;
		static bool load_trustedpeer;
		static std::string load_trustedpeer_file;

		static bool udpListenerOnly;
};


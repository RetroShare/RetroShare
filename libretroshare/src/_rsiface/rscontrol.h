#ifndef RSCONTROL_H
#define RSCONTROL_H

// Class RsControl - True Virtual

class RsControl /* The Main Interface Class - for controlling the server */
{
        public:

                RsControl(RsIface &i, NotifyBase &callback);


                virtual ~RsControl();

                /* Real Startup Fn */
                virtual int StartupRetroShare() = 0;

                /****************************************/

                /* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
                virtual int 	SetInChat(std::string id, bool in) = 0;		/* friend : chat msgs */
                virtual int 	SetInMsg(std::string id, bool in)  = 0;		/* friend : msg receipients */
                virtual int 	SetInBroadcast(std::string id, bool in) = 0;	/* channel : channel broadcast */
                virtual int 	SetInSubscribe(std::string id, bool in) = 0;	/* channel : subscribed channels */
                virtual int 	SetInRecommend(std::string id, bool in) = 0;	/* file : recommended file */
                virtual int 	ClearInChat() = 0;
                virtual int 	ClearInMsg() = 0;
                virtual int 	ClearInBroadcast() = 0;
                virtual int 	ClearInSubscribe() = 0;
                virtual int 	ClearInRecommend() = 0;

                virtual bool 	IsInChat(std::string id) = 0;		/* friend : chat msgs */
                virtual bool 	IsInMsg(std::string id) = 0;		/* friend : msg recpts*/

                /****************************************/
                /* Config */

                virtual int     ConfigSetDataRates( int totalDownload, int totalUpload ) = 0;
                virtual int     ConfigGetDataRates( float &inKb, float &outKb) = 0;
                virtual	int 	ConfigSetBootPrompt( bool on ) = 0;
                virtual void    ConfigFinalSave( ) 			   = 0;
                virtual void 	rsGlobalShutDown( )			   = 0;

                /****************************************/

                NotifyBase &getNotify();
                RsIface    &getIface();

        private:
                NotifyBase &cb;
                RsIface    &rsIface;
};

#endif // RSCONTROL_H

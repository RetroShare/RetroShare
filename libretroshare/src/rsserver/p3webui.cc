/*******************************************************************************
 * libretroshare/src/rsserver: p3webui.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2019-2019 Cyril Soler                                             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "p3webui.h"

#include <thread>
#include <iostream>
#include <fstream>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <restbed>
#include "util/rsthreads.h"
#include "util/rsdebug.h"
#include "retroshare/rswebui.h"

#define TEXT_HTML   0
#define TEXT_CSS    1
#define TEXT_SVG    2

#define DEBUG_RS_WEBUI 1

RsWebUI *rsWebUI = new p3WebUI;

static constexpr char *mime_types[3] = {
	"text/html",
	"text/css",
	"image/svg+xml"
};

#ifdef WINDOWS_SYS
static std::string _base_directory = "data/webui";
#else
static std::string _base_directory = "/usr/share/retroshare/webui/";
#endif


template<int MIME_TYPE_INDEX> class handler
{
	public:
		static void get_handler( const std::shared_ptr< restbed::Session > session )
		{
			const auto request = session->get_request( );
			const std::string filename = request->get_path_parameter( "filename" );
			const std::string directory = request->get_path_parameter( "dir" );

			std::string resource_filename = _base_directory + directory + "/" + filename;
			std::cerr << "Reading file: \"" << resource_filename << "\"" << std::endl;
			std::ifstream stream( resource_filename, std::ifstream::in );

			if ( stream.is_open( ) )
			{
				const std::string body = std::string( std::istreambuf_iterator< char >( stream ), std::istreambuf_iterator< char >( ) );

				std::cerr << "  body length=" << body.length() << std::endl;
				const std::multimap< std::string, std::string > headers
				{
					{ "Content-Type", mime_types[MIME_TYPE_INDEX] },
					{ "Content-Length", std::to_string( body.length( ) ) }
				};

				session->close( restbed::OK, body, headers );
			}
			else
			{
				std::cerr << "Could not open file " << resource_filename << std::endl;
				session->close( restbed::NOT_FOUND );
			}
		}
};

static void service_ready_handler( restbed::Service& )
{
    fprintf( stderr, "Hey! The service is up and running." );
}

class WebUIThread: public RsThread
{
public:
    WebUIThread()
    {
		_service = std::make_shared<restbed::Service>();
		_listening_port = 1984;
    }

    void runloop() override
	{
		auto resource1 = std::make_shared< restbed::Resource >( );
		resource1->set_paths( {
		                          "/{filename: index.html}",
		                          "/{filename: app.js}",
		                      }
		                      );
		resource1->set_method_handler( "GET", handler<TEXT_HTML>::get_handler );

		auto resource2 = std::make_shared< restbed::Resource >();
		resource2->set_paths( {
		                          "/{dir: css]/{filename: fontawesome.css}",
		                          "/{dir: css}/{filename: solid.css}",
		                          "/{filename: app.css}",
		                      } );
		resource2->set_method_handler( "GET", handler<TEXT_CSS>::get_handler );

		auto resource3 = std::make_shared< restbed::Resource >();
		resource3->set_paths( {
		                          "/{filename: retroshare.svg}",
		                      } );
		resource3->set_method_handler( "GET", handler<TEXT_SVG>::get_handler );

		auto settings = std::make_shared< restbed::Settings >( );
		settings->set_port( _listening_port );
		settings->set_default_header( "Connection", "close" );

		_service->publish( resource1 );
		_service->publish( resource2 );
		_service->publish( resource3 );

		_service->set_ready_handler( service_ready_handler );

        try
        {
			_service->start( settings );
        }
        catch(std::exception& e)
        {
            RsErr() << "Could  not start web interface: " << e.what() << std::endl;
            return;
        }
	}
    void stop()
    {
        _service->stop();

        RsThread::ask_for_stop();

        while(isRunning())
            sleep(1);
    }

    void setListeningPort(uint16_t p) { _listening_port = p ; }
    uint16_t listeningPort() const { return _listening_port;}

private:
    std::shared_ptr<restbed::Service> _service;
    uint16_t _listening_port;
};

p3WebUI::p3WebUI()
{
    _webui_thread = new WebUIThread;
}
p3WebUI::~p3WebUI()
{
    while(_webui_thread->isRunning())
    {
        stop();
        std::cerr << "Deleting webUI object while webUI thread is still running. Trying shutdown...." << std::endl;
		rstime::rs_usleep(1000*1000);
    }
    delete _webui_thread;
}

bool p3WebUI::restart()
{
    RsDbg() << "Restarting web interface listening on port " << _webui_thread->listeningPort() << std::endl;

    if(_webui_thread->isRunning())
        _webui_thread->stop();

    _webui_thread->start();
    return true;
}

bool p3WebUI::stop()
{
	_webui_thread->stop();
	return true;
}

void p3WebUI::setHtmlFilesDirectory(const std::string& html_dir)
{
    _base_directory = html_dir;
}
void p3WebUI::setListeningPort(uint16_t port)
{
    _webui_thread->setListeningPort(port);

    if(_webui_thread->isRunning())
		restart();
}

int p3WebUI::status() const
{
    if(_webui_thread->isRunning())
        return WEBUI_STATUS_RUNNING;
    else
        return WEBUI_STATUS_NOT_RUNNING;
}


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
#include "util/rsthreads.h"
#include "util/rsdebug.h"
#include "retroshare/rswebui.h"
#include "retroshare/rsjsonapi.h"

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
const std::string RsWebUI::DEFAULT_BASE_DIRECTORY = "data/webui";
#else
const std::string RsWebUI::DEFAULT_BASE_DIRECTORY = "/usr/share/retroshare/webui/";
#endif

static std::string _base_directory = RsWebUI::DEFAULT_BASE_DIRECTORY;


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

std::vector<std::shared_ptr<restbed::Resource> > p3WebUI::getResources() const
{
    std::vector<std::shared_ptr<restbed::Resource> > rtab;

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

	rtab.push_back(resource1);
	rtab.push_back(resource2);
	rtab.push_back(resource3);

    return rtab;
}


void p3WebUI::setHtmlFilesDirectory(const std::string& html_dir)
{
    _base_directory = html_dir;
}

int p3WebUI::status() const
{
    if(rsJsonAPI->status()==RsJsonAPI::JSONAPI_STATUS_RUNNING && rsJsonAPI->hasResourceProvider(this))
        return WEBUI_STATUS_RUNNING;
    else
        return WEBUI_STATUS_NOT_RUNNING;
}

void p3WebUI::setUserPassword(const std::string& passwd)
{
#ifdef RS_JSONAPI
    std::cerr << "Updating webui token with new passwd \"" << passwd << "\"" << std::endl;

    if(!rsJsonAPI->authorizeUser("webui",passwd))
        std::cerr << "(EE) Cannot register webui token. Some error occurred when calling authorizeUser()" << std::endl;
#else
	std::cerr << "(EE) JsonAPI is not available in this buildof Retroshare! Cannot register a user password for the WebUI" << std::endl;
#endif
}

bool p3WebUI::restart()
{
    rsJsonAPI->registerResourceProvider(this);

    return rsJsonAPI->restart();
}

bool p3WebUI::stop()
{
    rsJsonAPI->unregisterResourceProvider(this);

    if(rsJsonAPI->status()==RsJsonAPI::JSONAPI_STATUS_RUNNING)
		return rsJsonAPI->restart();
    else
        return true;
}



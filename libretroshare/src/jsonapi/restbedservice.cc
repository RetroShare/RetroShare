/*******************************************************************************
 * libretroshare/src/jsonapi/: restbedservice.cc                               *
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

#include "util/rsthreads.h"
#include "util/rsdebug.h"
#include "restbedservice.h"

class RestbedThread: public RsThread
{
public:
    RestbedThread()
    {
		_service = std::make_shared<restbed::Service>();	// this is a place holder, in case we request some internal values.
		_listening_port = 1984;
		_binding_address = "127.0.0.1";
    }

    void runloop() override
	{
        if(_resources.empty())
        {
            RsErr() << "(EE) please call RestbedService::setResources() before launching the service!" << std::endl;
            return;
        }
		auto settings = std::make_shared< restbed::Settings >( );
		settings->set_port( _listening_port );
		settings->set_bind_address( _binding_address );
		settings->set_default_header( "Connection", "close" );

		if(_service->is_up())
        {
            std::cerr << "(II) WebUI is already running. Killing it." << std::endl;
            _service->stop();
        }

		_service = std::make_shared<restbed::Service>();

        for(auto& r:_resources)
			_service->publish( r );

        try
        {
			std::cerr << "(II) Starting restbed service on port " << std::dec << _listening_port << " and binding address \"" << _binding_address << "\"" << std::endl;
			_service->start( settings );
        }
        catch(std::exception& e)
        {
            RsErr() << "Could  not start web interface: " << e.what() << std::endl;
            return;
        }

        std::cerr << "(II) restbed service stopped." << std::endl;
	}
    void stop()
    {
        _service->stop();

        RsThread::ask_for_stop();

        while(isRunning())
        {
			std::cerr << "(II) shutting down restbed service." << std::endl;
            rstime::rs_usleep(1000*1000);
        }
    }

    void setListeningPort(uint16_t p) { _listening_port = p ; }
    void setBindAddress(const std::string& bindAddress) { _binding_address = bindAddress ; }
    void setResources(const std::vector<std::shared_ptr<restbed::Resource> >& r) { _resources = r ; }
    uint16_t listeningPort() const { return _listening_port;}

private:
    std::shared_ptr<restbed::Service> _service;
	std::vector<std::shared_ptr<restbed::Resource> > _resources;

    uint16_t _listening_port;
    std::string _binding_address;
};

RestbedService::RestbedService()
{
    _restbed_thread = new RestbedThread();
}
RestbedService::~RestbedService()
{
    while(_restbed_thread->isRunning())
    {
        stop();
        std::cerr << "Deleting webUI object while webUI thread is still running. Trying shutdown...." << std::endl;
		rstime::rs_usleep(1000*1000);
    }
    delete _restbed_thread;
}

bool RestbedService::restart()
{
    RsDbg() << "Restarting restbed service listening on port " << _restbed_thread->listeningPort() << std::endl;

    if(_restbed_thread->isRunning())
        _restbed_thread->stop();

    _restbed_thread->setResources(getResources());
    _restbed_thread->start();
    return true;
}

bool RestbedService::stop()
{
	_restbed_thread->stop();
	return true;
}
bool RestbedService::isRunning() const
{
    return _restbed_thread->isRunning();
}
void RestbedService::setListeningPort(uint16_t port)
{
    _restbed_thread->setListeningPort(port);

    if(_restbed_thread->isRunning())
		restart();
}

void RestbedService::setBindAddress(const std::string& bind_address)
{
    _restbed_thread->setBindAddress(bind_address);

    if(_restbed_thread->isRunning())
		restart();
}


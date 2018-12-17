#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Util/XMLConfiguration.h"
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/AutoPtr.h"
#include "MitaiSub.h"
#include <iostream>

using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::OptionCallback;
using Poco::Util::HelpFormatter;
using Poco::Util::LayeredConfiguration;
using Poco::Util::XMLConfiguration;
using Poco::Task;
using Poco::TaskManager;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::AutoPtr;


NotificationQueue NQ_MiTAI;

class SampleTask: public Task
{
public:
	SampleTask(): Task("SampleTask"),_queue(NQ_MiTAI)
	{
	}
	
	void runTask()
	{
		Application& app = Application::instance();
		AutoPtr<Notification> pNf(_queue.waitDequeueNotification(100));
		while (1)
		{
			/*WorkNotification* pWorkNf = dynamic_cast<WorkNotification*>(pNf.get());

			if (pWorkNf)
			{
				// do some work
			}
			*/
			MiTAINotification* _mitai = dynamic_cast<MiTAINotification*>(pNf.get());
			if (_mitai)
			{
				Application::instance().logger().information(_mitai->data());
			}
			pNf = _queue.waitDequeueNotification(100);
			if(isCancelled()) return;
		}
		
		
	}
private:
	NotificationQueue& _queue;
};

class MitelMonitor: public ServerApplication
{
public:
	MitelMonitor(): 
		_helpRequested(false),
		_SrvManagerRequested(false)
	{
		Application::instance().addSubsystem(new MiTAISub);
	}
	
	~MitelMonitor()
	{
	}

protected:
	void initialize(Application& self)
	{
		if (_helpRequested || _SrvManagerRequested) return;
		loadConfiguration(); // load default configuration files
		try{

			ServerApplication::initialize(self);
		}
		catch (...)
		{
			logger().fatal("Failed to initialize... Exiting...");
			ServerApplication::uninitialize();
			ServerApplication::terminate();
			return;
			
		}
		logger().information("starting up");
	}
		
	void uninitialize()
	{
		if (_helpRequested || _SrvManagerRequested ) return;


		logger().information("shutting down");
		ServerApplication::uninitialize();
	}

	void defineOptions(OptionSet& options)
	{
		ServerApplication::defineOptions(options);
		
		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false)
				.callback(OptionCallback<MitelMonitor>(this, &MitelMonitor::handleHelp)));
	}

	void handleHelp(const std::string& name, const std::string& value)
	{
		_helpRequested = true;
		displayHelp();
		stopOptionsProcessing();
	}

	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("MitelMonitor - Connecting to multiple 3300 ICPs and monitors state of trunks and DNs");
		helpFormatter.format(std::cout);
	}
	void handleOption(const std::string& name, const std::string& value)
	{
		if(name == "registerService" || name == "unregisterService") _SrvManagerRequested = true;
		ServerApplication::handleOption(name,value);
	}
	int main(const std::vector<std::string>& args)
	{
		if (!(_helpRequested || _SrvManagerRequested || !this->initialized()))
		{
			//Start notification queue monitor (Replace Sample Task)
			TaskManager tm;
			tm.start(new SampleTask);
			//start MiTAISub
			MiTAISub &_mitai = this->getSubsystem<MiTAISub>();
			_mitai.run();
			logger().information("Waiting for termination");
			waitForTerminationRequest();
			tm.cancelAll();
			tm.joinAll();
			
		}
		return Application::EXIT_OK;
	}
	
private:
	bool _helpRequested;
	bool _SrvManagerRequested;
};


POCO_SERVER_MAIN(MitelMonitor)
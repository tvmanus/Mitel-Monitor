# Mitel-Monitor
Mitel Monitor is the application based on Mitel Trunk Monitor application. The goal is to extend functionality and make the application run as windows service.
Following is the preliminary list of features
- Running as windows service and as a console application
- XML configuration file
- MiSDK4 MiTAI connector
- Embedded web server with websocket streaming. Serving client web page.
- Embedded TCP Policy server for flash XML Socket (required by Adobe)
Future features
- Users authentication on XML Socket
- Role based access and customizable views based on stored profiles.
- The application infrastructure is implemented based on POCO framework. 
- The first layer is implemented as MitelMonitor class extended from Poco::ServerApplication class.
- Information about state of all monitored objects will be stored in a static class which is part of MiTAI Subsystem
- HTTPServer Subsystem
- Logging Subsystem

First of all the MiTAI subsystem should be implemented. 
It will require following configurations:
- IP Address of the local interface. For now we will not ry to detect it automatically.
- Information about monitored objects
- MiTAI private logging directory

In order to satisfy these requirements need to implement configuration loading. Let's use XML configuration format.
The configuration file name will be MitelMonitor.xml

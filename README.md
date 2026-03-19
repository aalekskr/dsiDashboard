# dsiDashboard
Native DSi Home Assistant implementation.

You have to compile the .nds file by yourself.
You have to install **devkitpro** (you need devkitARM) to compile.
Make sure to apply your **customization to main.c!**

Uses a long lived access token to get the data from the HA instance in an local network.
It is currently only **text based**, no graphical interface.
Future implementation of an interactive GUI is planned.
Displays up to 8 entity states on the lower screen, the weather, temperature and time.
Make sure to set up your DSi WiFi the correct way. **(WFC Settings -> Advanced Setup (For WPA2))**


For it to work, you need to:
- Change the host ip and port to your specific one
- Insert your own long lived access token 
- change the entities to those you want to have displayed
- change the weather entity if yours differs from mine
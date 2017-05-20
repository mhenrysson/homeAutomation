# homeAutomation - Mosquitto
Docker Mosquitto server

Requires docker-compose to be installed on container host  

Start: sudo docker-compose up -d  

Ensure mosquitto user has proper permissions on mosquitto directory (easy but unsecure: chmod -R 777 <mosquitto directory>)  

Change or remove IP for container host ports in docker-compose.yaml as appropriate to your network setup  
Volumes specified with :Z option for SELinux compatibility. Drop if not using SELinux.  

No security measures besides firewall rules are in place. Anyone able to access the host ports can pub/sub to Mosquitto.  

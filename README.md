# IoTSASTokenGenerate
C and C++ sample programs that demonstrate how to generate a SAS Token from a device connection string in order to connect a device to an Azure IoT hub without using the Microsoft SDK.

When connecting to an Azure IoT hub using the standard Microsoft SDKs one only needs to provide a device connection string. 
This string is used to create a SAS token which is subsequently used to 
authenticate the device with the Azure IoT hub.

However, there instances that you may wish to access the Azure IoT hub using a mechanism other than the Microsoft SDKs. This requires 
one to create the SAS token in order to authenticate with the hub. One can do this by generating the SAS token with the Device Explorer (https://github.com/Azure/azure-iot-sdk-csharp/tree/master/tools/DeviceExplorer)
or the iothub-explorer node tool (https://github.com/azure/iothub-explorer) but neither of these is a suitable long term solution because 
the SAS token has a finite lifetime. When it expires one will no longer be able to use it to authenticate with the Azure IoT hub. 

This project presents an example of how to generate your own SAS token from a device connection string. I have attempted to minimize the 
number of dependencies in it by including my own implementations of URL encoding and Base 64 encoding and decoding. This code is based upon 
the code that I used to generate the SAS token in the ESP8266 sample that uses a third party MQTT library.

**This is sample code only. It doesn't do much error checking and it might leak memory. It is provided for the purposes of demonstration only.**

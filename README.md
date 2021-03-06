# h-box
A UPnP proxy that publishes the local UPnP devices to remote home networks

### PURPOSE:

All the current upnp devices including Control Points, Media Server, Media Rendrrer, etc. run only in the local network. This Software allows us to run upnp devices that are in the remote network and/or are behind NAT.

This Software uses xmpp protocol to send and receivce control message like executing some actions on the remote upnp device. It uses TCP over HIP to send and receive media files.

### INSTALLATION:

Check the ```INSTALL``` file.

### SAMPLE CONFIGURATION:

First of all we need a xmpp server and id. We used gmail as xmpp server in this case. The xmpp id along with the password has to added in the conf/example.conf file of the software directory in the following format. 
```
	[userinfo]
	username= xxx@gmail.com/hbox
	password= xxx
```

Now install 'gupnp-tools' from the standard repository in debian systems. After installing this software 'gupnp-universal-cp' command will give us a standard control point using which we would be able to explore upnp device and execute actions on them.

Now run the hbox sotware in both the networks that want to share their upnp devices. 'gupnp-universal-cp' would show us a device name 'HBOX-device'. This device contains all the devices that are currently present in the remote network as its chil devices. We can go ahead and execute any actions on them and get the output. As mentioned before these actions/responses would be send and received over xmpp but if we want to play some media file in our local network that are served in a Media Server located in the remote nework then we can't use xmpp. We need direct connection in that case. But these are all internal things and the user won't be able to see the difference while playing the local and the remote media files. Following is way to run a media file that served in the remote network. This way is somewhat clumy but from a developsers' perspective it will give the insite to the actuall upnp AV protocol.

1. Start the 'gupnp-universal-cp' in the media rendrer side. 
2. Start the Hbox in both the network. 'gupnp-universal-cp' should be able to see the 'HBOX-device' in the list of the available devices.
3. Expand the Media Server device and execute the 'Browse' action with the appropriate 'in' arguments. e.g. ```ObjectID = '28', BrowseFlag = 'BrowseMetadata'```. Output would be available in the 'out' arguments e.g. ```Result = '<DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/"><item id="28" parentID="4" restricted="1"><dc:title>djsk-everything_beautiful.mp3</dc:title><upnp:class>object.item.audioItem.musicTrack</upnp:class><res protocolInfo="http-get:*:audio/mpeg:*" size="3825579">http://192.168.1.31:49153/content/media/object_id/28/res_id/0/ext/file.mp3</res></item></DIDL-Lite>'```
4. URL ```'http://192.168.1.31:49153/content/media/object_id/28/res_id/0/ext/file.mp3'``` in the 'Result' section is the location where the media file is shared.
5. Using 'gupnp-universal-cp' explore the Media Rendrer local device and execute the 'Prepare for Connection' action in the 'ConnectionManager' service with the appropriate 'in' arguments e.g. ```RemoteProtocolInfo = 'http-get:*:audio/mpeg:*'``` (mentioned in the 'Result'), ```PeerConnectionManager = 'http://130.233.194.245:4004/cm.xml'``` (we can see this in the 'gupnp-universal-cp' when we click on the communication manager service.), ```PeerConnectionID = '1'``` (Any id which is not used till now), Direction = 'input' (we are rendrrer and we would input the nedia file).
6. Using 'gupnp-universal-cp' explore the Media Rendrer local device and execute the 'SetAVTransportURI' action in the 'AVTransport' service with the appropriate 'in' arguments e.g. ```InstaceID = '1'``` (id which we set in the previous step), ```CurrentURI = 'http://192.168.1.31:49153/content/media/object_id/28/res_id/0/ext/file.mp3'``` (location of the media file).
7. Using 'gupnp-universal-cp' explore the Media Rendrer local device and execute the 'Play' action in the 'AVTransport' service with the appropriate 'in' arguments e.g. InstaceID = '1' and Speed = '1' (1 is the normal speed). This action would play the file served in the remote upnp Meida Server device.

Ther are many open source automated Control Point software that are available to automate the searching of the media file on the media server device so that we don't have to search for the 'ObjectId' while executing the 'Browse' action. These Control Point are even one step ahead in the sense that we don't have to execute any action manually. But there is one problem with all these control points, they only search for the upnp device that are root device by itself. In our case 'HBOX-device' is the root device and the media server device is the child device so our remote moedia server deivce don't get searched by these control points. Following are few such control points.

* Liea -- http://leia.sommerfeldt.f-m.fm/
* KinskyDesktop -- http://oss.linn.co.uk/trac
* KinskyPda -- http://oss.linn.co.uk/trac
* KinskyClassic -- http://oss.linn.co.uk/trac
* Zhaan -- http://maemo.org/packages/view/zhaan/

*Media Rendrer Used*:
* RhythmBox -- http://projects.gnome.org/rhythmbox/
* GMediaRender -- http://gmrender.nongnu.org/
* vlc -- http://www.videolan.org/vlc/

*Media Server Used*:
* MeidaTomb -- http://mediatomb.cc/
* Rygel -- https://webstats.gnome.org/Rygel
* RhythmBox -- http://projects.gnome.org/rhythmbox/

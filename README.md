# sleepmotion-ble

Two implementations of a BLE controller for a dreams sleep motion bed. One running on a Raspberry pi zero, the other on an Atom lite ESP32 microprocessor.

Both implementations provide a rest interface that can be integrated with a home assistant using the rest command integration (note: the ESP32 implementation uses get rather than post).

```
rest_command:
  bed_flat:
    url: "http://192.168.1.248:5000/flat"
    method: post
  bed_zerog:
    url: "http://192.168.1.248:5000/zerog"
    method: post
  bed_light:
    url: "http://192.168.1.248:5000/light"
    method: post
  bed_situp:
    url: "http://192.168.1.248:5000/situp"
    method: post
  bed_feetdown:
    url: "http://192.168.1.248:5000/feetdown"
    method: post
```

Once available within home assistant voice integration can be set up as per standard home assistant features.
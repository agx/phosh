#!/usr/bin/env python2

import dbus
import dbus.service
import dbus.mainloop.glib

import gobject

class Service(dbus.service.Object):
    connector = 'HDMI-1'
    resources = (1,     # serial
                 [      # crtc
                    [ 
                       0,    # serial
                       0,    # winsys_id
                       0, 0, 768, 1024,  # x, y, width, height
                       0,                # current mode
                       0,                # current transform (according to wayland proto
                       [0],              # all possible transforms
                       {},
                    ],
                 ],
                 [                 # outputs
                    [
                       0,          # serial
                       0,          # winsys_id
                       0,          # current_crtc
                       [0],        # possible_crtcs
                       connector,  # connector
                       [1],        # valid modes
                       [1],        # valid clones
                       {'vendor': 'puri.sm'},  # properties
                    ],
                 ],
                 [                 # modes
                    [
                       0,          # serial
                       0,          # XID
                       768, 1024,  # width, height
                       60,         # frequency
                       0,          # flags
                    ],
                 ],
                 768,              # max_width
                 10240,            # max_height
    )

    def run(self):
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        bus_name = dbus.service.BusName("org.gnome.Mutter.DisplayConfig", dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, "/org/gnome/Mutter/DisplayConfig")

        self._loop = gobject.MainLoop()
        print("Mocking mutter DisplayConfig DBus API")
        self._loop.run()

    @dbus.service.method("org.gnome.Mutter.DisplayConfig",
                         in_signature='',
                         out_signature='ua(uxiiiiiuaua{sv})a(uxiausauaua{sv})a(uxuudu)ii')
    def GetResources(self):
        print("GetResouces called")
        return self.resources

    @dbus.service.method("org.freedesktop.DBus.Properties", in_signature='ssv', out_signature='')
    def Set(self, s1, s2, v):
        print("Set called with %s %s" % (s1, s2))
        return


if __name__ == "__main__":
    Service("This is the service").run()

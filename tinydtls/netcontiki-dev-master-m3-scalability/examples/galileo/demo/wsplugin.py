# A ws demo plugin module

class DemoPlugin:

    def get_commands(this, wsdemo):
        return [];

    def handle_cmd(this, wsdemo, cmd):
        print "Default handler called - will do nothing: ", cmd

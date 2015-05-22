#include <stdio.h>
#include <signal.h>

#include <alljoyn/Init.h>
#include <alljoyn/version.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Status.h>

using namespace std;
using namespace ajn;
using namespace qcc;

static const char* INTERFACE_NAME = "org.alljoyn.Bus.sample";
static const char* SERVICE_NAME = "org.alljoyn.Bus.sample";
static const char* SERVICE_PATH = "/sample";
static const SessionPort SERVICE_PORT = 25;
static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig)
{
	QCC_UNUSED(sig);
	s_interrupt = true;
}

class MyBusListener : public BusListener, public SessionPortListener {
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s.\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != SERVICE_PORT) {
            printf("Rejecting join attempt on unexpected session port %d.\n", sessionPort);
            return false;
        }
        printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x).\n",
               joiner, opts.proximity, opts.traffic, opts.transports);
        return true;
    }
};

class BasicSampleObject : public BusObject {
  public:
    BasicSampleObject(BusAttachment& bus, const char* path) :
        BusObject(path)
    {
        /** Add the test interface to this object */
        const InterfaceDescription* exampleIntf = bus.GetInterface(INTERFACE_NAME);
        assert(exampleIntf);
        AddInterface(*exampleIntf);

        /** Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { exampleIntf->GetMember("cat"), static_cast<MessageReceiver::MethodHandler>(&BasicSampleObject::Cat) }
        };
        QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        if (ER_OK != status) {
            printf("Failed to register method handlers for BasicSampleObject.\n");
        }
    }

    void ObjectRegistered()
    {
        BusObject::ObjectRegistered();
        printf("ObjectRegistered has been called.\n");
    }


    void Cat(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);
        /* Concatenate the two input strings and reply with the result. */
        qcc::String inStr1 = msg->GetArg(0)->v_string.str;
        qcc::String inStr2 = msg->GetArg(1)->v_string.str;
        qcc::String outStr = inStr1 + inStr2;

        MsgArg outArg("s", outStr.c_str());
        QStatus status = MethodReply(msg, &outArg, 1);
        if (ER_OK != status) {
            printf("Ping: Error sending reply.\n");
        }
    }
};

/** Wait for SIGINT before continuing. */
void WaitForSigInt(void)
{
    while (s_interrupt == false) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
}

static BusAttachment* s_msgBus = NULL;
static MyBusListener* s_busListener;

int main(int argc, char** argv, char** envArg)
{
	QCC_UNUSED(argc);
	QCC_UNUSED(argv);
	QCC_UNUSED(envArg);

	if(AllJoynInit() != ER_OK) {
		printf("AllJoyn Init failed.\n");
		return 1;
	}

	printf("AllJoyn version = %s.\n", ajn::GetVersion());
	printf("AllJoyn build info = %s.\n", ajn::GetBuildInfo());

	/* Install SIGINT handler */
	signal(SIGINT, SigIntHandler);

	QStatus status = ER_OK;
	BasicSampleObject* basicObj;

	/* Create message bus */
	s_msgBus = new BusAttachment("SampleApp", true);
	if(s_msgBus) {
		InterfaceDescription* interfaceDesp = NULL;
		s_msgBus->CreateInterface(INTERFACE_NAME, interfaceDesp);
		interfaceDesp->AddMethod("cat", "ss", "s", "inStr1,inStr2,outStr", 0);
		interfaceDesp->Activate();

		s_msgBus->RegisterBusListener(*s_busListener);

		s_msgBus->Start();

		basicObj = new BasicSampleObject(*s_msgBus, SERVICE_PATH);

		s_msgBus->RegisterBusObject(*basicObj);

		s_msgBus->Connect();

        /*
         * Advertise this service on the bus.
         * There are three steps to advertising this service on the bus.
         * 1) Request a well-known name that will be used by the client to discover
         *    this service.
         * 2) Create a session.
         * 3) Advertise the well-known name.
         */
	    const uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
	    s_msgBus->RequestName(SERVICE_NAME, flags);

	    const TransportMask SERVICE_TRANSPORT_TYPE = TRANSPORT_ANY;

	    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, SERVICE_TRANSPORT_TYPE);
	    SessionPort sp = SERVICE_PORT;
	    s_msgBus->BindSessionPort(sp, opts, *s_busListener);

	    s_msgBus->AdvertiseName(SERVICE_NAME, SERVICE_TRANSPORT_TYPE);

	    /* Perform the service asynchronously until the user signals for an exit. */
	    WaitForSigInt();

	} else {
		status = ER_OUT_OF_MEMORY;
	}

	/* Clean up */
	delete s_msgBus;
	s_msgBus = NULL;
	delete basicObj;
	basicObj = NULL;

	printf("Basic service exiting with status 0x%04x (%s).\n", status,
			QCC_StatusText(status));

	return (int) status;
}

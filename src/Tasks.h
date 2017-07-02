//int processLoad = 0;

class WebserverProcess : public Process {
public:
    WebserverProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {}

protected:
    virtual void service() {
        //webserver.tick();
    }
};

class HardwareProcess : public Process {
public:
    HardwareProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {}

protected:
    virtual void service() {
      //hardware.tick();
      /*
      if(isOTAServiceAttached) {
        ArduinoOTA.handle();
      }
      */
    }
};

class TimedEventProcess : public Process {
public:
    TimedEventProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {}

protected:
    virtual void service() {
        //timedEvent();

        //processLoad = getLoadPercent();
    }
};

Scheduler sched;

WebserverProcess webserverProc(sched, HIGH_PRIORITY, SERVICE_CONSTANTLY, RUNTIME_FOREVER);
HardwareProcess hardwareProc(sched, HIGH_PRIORITY, SERVICE_CONSTANTLY, RUNTIME_FOREVER);
TimedEventProcess timedEventProc(sched, LOW_PRIORITY, 1000, RUNTIME_FOREVER);

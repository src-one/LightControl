interface Inject {
    isDevEnv: boolean;
    cdn: string;
    fwVersion: string;
    hwVersion: string;
    chipId: string;
    macId: string;
    isAppResourceLoaded: boolean;
}

interface Window {
    __karma__?: any;
    jQuery: any;
    inject: Inject;
}

declare module "window" {
    export = Window;
}

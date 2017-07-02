interface NoopFunction {
    (): void;
}

interface Karma {
    files: {};
    loaded: NoopFunction;
    start: NoopFunction;
}

declare var __karma__: Karma;

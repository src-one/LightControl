import {Observable} from 'rxjs/Observable';

type OnFail = (error: Error) => void;

export class ObservablePending<T> {
    public static resolve<T>(value?: any): ObservablePending<T> {
        const pending = new ObservablePending<T>(undefined);
        pending.done(value);
        return pending;
    }

    public static reject<T>(error): ObservablePending<T> {
        const pending = new ObservablePending<T>(undefined);
        pending.fail(error);
        return pending;
    }

    public pending: boolean = true;
    public error: Error;
    public response: T;
    public done: (response: T) => void;
    public fail: (error: Error) => Observable<{}>;
    public onFail: (callback: OnFail) => (error: Error) => Observable<{}>;

    constructor(public value: any) {
        this.done = (response: T): void => {
            this.pending = false;
            this.response = response;
        };
        this.onFail = (callback: OnFail) => {
            return (error: Error) => {
                setTimeout(function() {
                    callback(error);
                }, 1);

                return this.fail(error);
            };
        };
        this.fail = (error: Error) => {
            this.done(undefined);
            this.error = error;
            return Observable.empty();
        };
    }

    public getErrorBody<R>(): R {
        /*
        if (this.error instanceof Response) {
            return (<Response>this.error).json();
        }
        */
        return null;
    }
}

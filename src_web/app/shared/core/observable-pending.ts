import {Observable} from 'rxjs/Observable';

export class ObservablePending<T> {
    pending: boolean = true;
    error: Error;
    response: T;
    done: (response: T) => void;
    fail: (error: Error) => Observable<{}>;
    onFail: (callback: Function) => (error: Error) => Observable<{}>;

    static resolve<T>(value = undefined): ObservablePending<T> {
        const pending = new ObservablePending<T>(undefined);
        pending.done(value);
        return pending;
    }

    static reject<T>(error): ObservablePending<T> {
        const pending = new ObservablePending<T>(undefined);
        pending.fail(error);
        return pending;
    }

    constructor(public value: any) {
        this.done = (response: T): void => {
            this.pending = false;
            this.response = response;
        };
        this.onFail = (callback: Function) => {
            return (error: Error) => {
                setTimeout(function() {
                    callback(error);
                }, 1);

                return this.fail(error);
            }
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

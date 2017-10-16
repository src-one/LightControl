import {Observable} from 'rxjs/Observable';
import {ReplaySubject} from 'rxjs/ReplaySubject';
import {ObservablePending} from './observable-pending';
import {Subscription} from 'rxjs/Subscription';
import {Subject} from 'rxjs/Subject';
import {OnDestroy} from '@angular/core';
import {ngOnDestroy} from './ng-on-destroy';

/**
 * Observable Handling
 *
 * - handles errors
 * - handles component teardown
 * - handles preloading state
 *
 * Example code block:
 *
 *
 * ObservableHandler.from<MyModel[]>(this.page, this)
 * .setApi(page => MyService.getData(page))
 * .onPending(pending => this.data = pending)
 * .subscribe();
 *
 *
 * Example angular2 component:
 *
 *
 * class MyService {
 *     static getData(page: number): Observable<MyModel> {
 *         return Observable.create(function(subscriber) {
 *             subscriber.next([
 *                 new MyModel(),
 *                 new MyModel(),
 *             ]);
 *         }));
 *     }
 * }
 *
 * class MyModel {
 *     name: string = 'foo';
 * }
 *
 * @Component({
 *        selector: 'my-component',
 *        template: `
 *            <div *ngIf="data?.pending">Loading ...</div>
 *            <div *ngIf="data?.error">Couldn't Load Data</div>
 *            <div *ngFor="let item of data?.response">{{ item.name }}</div>
 *        `
 *    })
 * export class MyComponent implements OnInit {
 *     public data: ObservablePending<MyModel[]>;
 *     private page = new EventEmitter<number>();
 *
 *     ngOnInit() {
 *         ObservableHandler.from<ProductReviewResponse>(this.page, this)
 *             .setApi(page => MyService.getData(page))
 *             .onPending(pending => this.data = pending)
 *             .subscribe();
 *
 *         this.page.emit(1);
 *     }
 * }
 *
 */

const noop = function() {
};

export class ObservableHandler<T> {
    public static take<T>(value: any = null, component: {} = null) {
        const subject = new ReplaySubject(1);
        subject.next(value);

        return new ObservableHandler<T>()
            .setInput(subject)
            .setComponent(component);
    }

    public static from<T>(input: Observable<any> = null, component: {} = null) {
        return new ObservableHandler<T>()
            .setInput(input)
            .setComponent(component);
    }

    public static getDelayedRetry(retries: number, delay: number, doNotRetryOnStatusCode: number[]) {
        return (errors) => errors.scan(function(retry, error) {
            if (retry >= retries || doNotRetryOnStatusCode.indexOf(error.status) !== -1) {
                throw error;
            }
            return retry + 1;
        }, 0).delay(delay);
    }
    private apiCallback: (value) => Observable<T>;
    private pendingCallback: (pending) => any;
    private errorCallback: (error, value) => any;
    private retries: number = 0;
    private input: Observable<any>;
    private throttle: number = 0;
    private component: OnDestroy;

    public testError(message: string = 'testError'): ObservableHandler<T> {
        return this.setApi(() => Observable.throw(new Error(message)));
    }

    public testSuccess(data: any, delay: number = 1): ObservableHandler<T> {
        return this.setApi(() => Observable.timer(delay).map(() => data));
    }

    public setInput(input: Observable<any>): ObservableHandler<T> {
        this.input = input;
        return this;
    }

    public setComponent(component: {}): ObservableHandler<T> {
        this.component = component as OnDestroy;
        return this;
    }

    public setApi(api: (value) => Observable<T>): ObservableHandler<T> {
        this.apiCallback = api;
        return this;
    }

    public setRetries(retries: number): ObservableHandler<T> {
        this.retries = retries;
        return this;
    }

    public setThrottle(throttle: number): ObservableHandler<T> {
        this.throttle = throttle;
        return this;
    }

    public onPending(callback: (pending: ObservablePending<T>) => any): ObservableHandler<T> {
        this.pendingCallback = callback;
        return this;
    }

    public onError(callback: (error, value) => any): ObservableHandler<T> {
        this.errorCallback = callback;
        return this;
    }

    public subscribe(subscriber: (value: T) => any = noop): Subscription {
        const destroy = new Subject<void>();
        const config = Object.assign({
            pendingCallback: noop,
            errorCallback: noop,
        }, this) as any;

        if (!this.apiCallback) {
            throw new Error('ObservableHandler - api must be set with "setApi(api: (value) => Observable<T>)"');
        }

        let stream = config.input
            .map(function(value: any): ObservablePending<T> {
                const next = new ObservablePending<T>(value);
                destroy.next(undefined);
                config.pendingCallback(next);
                return next;
            });

        if (config.throttle) {
            stream = stream.auditTimeImmediate(config.throttle);
        }

        stream = stream.switchMap(function(next: ObservablePending<T>) {
            return config.apiCallback(next.value)
                .takeUntil(destroy)
                .retryWhen(ObservableHandler.getDelayedRetry(config.retries, 1500, [403, 409, 400, 404]))
                .catch(next.onFail((error) => config.errorCallback(error, next.value)))
                .do(next.done);
        });

        const subscription = stream.subscribe(subscriber);

        ngOnDestroy(() => {
            if (typeof subscription === 'object' && !subscription.closed) {
                subscription.unsubscribe();
            }
        }, config.component);

        return subscription;
    }
}

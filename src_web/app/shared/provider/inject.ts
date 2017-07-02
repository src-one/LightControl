import {NgModule} from '@angular/core';

/**
 * transforms values in `window.inject` into angular2 providers
 * provide them to your application/component so you can inject them via angular2 dependency injection:
 *
 * class MyClass {
 *     constructor(@Inject('name') variable) {}
 * }
 *
 * - declare the global inside definitions/window.d.ts for typescript
 * - write the value into `window.inject.foo`
 * - write factory `export function _Foo() { return inject.foo }`
 * - write provider in injectModule `{provide: 'foo', useFactory: _Foo}`
 */

export const Inject: Inject = window.inject || (<any>{});

export function _isDevEnv() {
    return Inject.isDevEnv;
}

export function _cdn() {
    return Inject.cdn;
}

export function _fwVersion() {
    return Inject.fwVersion;
}

export function _hwVersion() {
    return Inject.hwVersion;
}

export function _chipId() {
    return Inject.chipId;
}

export function _macId() {
    return Inject.macId;
}

@NgModule({
    providers: [
        {provide: 'isDevEnv', useFactory: _isDevEnv},
        {provide: 'cdn', useFactory: _cdn},
        {provide: 'fwVersion', useFactory: _fwVersion},
        {provide: 'hwVersion', useFactory: _hwVersion},
        {provide: 'chipId', useFactory: _chipId},
        {provide: 'macId', useFactory: _macId},
    ],
})
export class InjectModule {
}

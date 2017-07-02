import {OnDestroy} from '@angular/core';

export function ngOnDestroy(onDestroy: () => any, instance: OnDestroy = null) {
    if (!instance) {
        return;
    }

    const original = instance.ngOnDestroy;
    instance.ngOnDestroy = function() {
        try {
            original && original.apply(instance, arguments);
        } catch (error) {
            window.console && console.error(error);
        }
        onDestroy();
    };
}

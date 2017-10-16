import {Component, Input} from '@angular/core';

@Component({
    selector: 'preloader',
    template: `
        <div class="preloader">
            <div class="preloader__inner">
                <label>	●</label>
                <label>	●</label>
                <label>	●</label>
                <label>	●</label>
                <label>	●</label>
                <label>	●</label>

                <div class="preloader__caption" *ngIf="caption">
                    {{ caption }}
                </div>
            </div>
        </div>
    `,
})
export class PreLoaderComponent {
    @Input() public caption: string = '';
}

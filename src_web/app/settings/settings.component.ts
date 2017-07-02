import {Component, OnInit} from '@angular/core';

@Component({
    selector: 'settings',
    template: `
        <p>
            Settings...
        </p>
    `,
    styleUrls: ['./settings.component.scss'],
})
export class SettingsComponent implements OnInit {
    constructor() {
        // Do stuff
    }

    public ngOnInit() {
        console.log('Settings');
    }

}

import {Component} from '@angular/core';

@Component({
    selector: 'home',
    template: `
        <div class="container">
            <div class="panel">
                <channel></channel>

                <websocket-test></websocket-test>
            </div>
        </div>
    `,
})
export class HomeComponent {
}

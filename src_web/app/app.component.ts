import {Component} from '@angular/core';
import '../css/main.scss';

@Component({
    selector: 'app-root',
    template: `
        <header>
            <header-navigation></header-navigation>
        </header>
        <main>
            <router-outlet></router-outlet>
        </main>
        <footer>
        </footer>
    `,
})
export class AppComponent {
    constructor() {
    }
}

import {Component} from '@angular/core';
import '../css/main.scss';

@Component({
    selector: 'app-root',
    template: `
        <header>
            <header-navigation></header-navigation>
        </header>
        <main>
            <!--<h1>Hello from {{title}}!</h1>-->

            <!-- Images (and assets) are parsed and loaded from within the public directory -->
            <!--<img src="/images/angular.png">-->
            <router-outlet></router-outlet>
        </main>

        <!--<preloader></preloader>-->

        <!--<div class="progress">
            <div class="progress-bar" role="progressbar" aria-valuenow="60" aria-valuemin="0" aria-valuemax="100" style="width: 60%;">
                60%
            </div>
        </div>-->

        <footer>
            <!--<a [href]="url">Webpack Angular 2 Starter</a>-->
        </footer>
    `
})
export class AppComponent {
    constructor() {
    }
}

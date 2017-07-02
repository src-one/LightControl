import {RouterModule, Routes} from '@angular/router';
import {HomeComponent} from './home/home.component';
import {SettingsComponent} from './settings/settings.component';

const routes: Routes = [
    {path: '', component: HomeComponent},
    {path: 'settings', component: SettingsComponent}
];

export const routing = RouterModule.forRoot(routes);

import {ApplicationRef, NgModule} from '@angular/core';
import {BrowserModule} from '@angular/platform-browser';
import {HttpModule} from '@angular/http';
import {FormsModule} from '@angular/forms';
import {AppComponent} from './app.component';
import {HomeComponent} from './home/home.component';
import {SettingsComponent} from './settings/settings.component';
import {routing} from './app.routing';
import {createNewHosts, removeNgStyles} from '@angularclass/hmr';
import {HeaderNavigationComponent} from './shared/main/header-navigation.component';
import {WebSocketService} from './shared/service/websocket.service';
import {WebsocketTestComponent} from './home/websocket-test.component';
import {WebsocketTestService} from './home/websocket-test.service';
import {Filter} from './shared/helper/common-filter';
import {ChannelComponent} from './home/channel.component';
import {LightState} from './home/light.state';
import {LightService} from './shared/service/light.service';
import {SliderComponent} from './shared/widgets/slider.component';
import {PreLoaderComponent} from './shared/widgets/preloader.component';

@NgModule({
    imports: [
        BrowserModule,
        HttpModule,
        FormsModule,
        routing,
    ],
    declarations: [
        AppComponent,
        HomeComponent,
        ChannelComponent,
        SettingsComponent,
        PreLoaderComponent,
        HeaderNavigationComponent,
        SliderComponent,
        WebsocketTestComponent,
    ],
    providers: [
        LightService,
        WebSocketService,
        WebsocketTestService,

        LightState,

        // filters
        Filter,
    ],
    bootstrap: [AppComponent],
})
export class AppModule {
    constructor(public appRef: ApplicationRef) {
    }

    public hmrOnInit(store) {
        console.log('HMR store', store);
    }

    public hmrOnDestroy(store) {
        const cmpLocation = this.appRef.components.map((cmp) => cmp.location.nativeElement);
        // recreate elements
        store.disposeOldHosts = createNewHosts(cmpLocation);
        // remove styles
        removeNgStyles();
    }

    public hmrAfterDestroy(store) {
        // display new elements
        store.disposeOldHosts();
        delete store.disposeOldHosts;
    }
}

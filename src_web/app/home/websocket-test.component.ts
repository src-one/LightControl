import {Component, OnInit} from '@angular/core';
import {Subject} from 'rxjs/Rx';
import {WebSocketService} from '../shared/service/websocket.service';
import {LightState} from './light.state';

@Component({
    selector: 'websocket-test',
    template: `
        <div class="container">
            websocket component loaded...
            <div>Sent Message: {{ sentMessage }}</div>
            <div>Recv Message: {{ message }}</div>
        </div>
    `
})
export class WebsocketTestComponent implements OnInit {
    private socket: Subject<any>;
    //private counterSubscription: Subscription;
    private message: string;
    public sentMessage: string = "";

    constructor(websocketService: WebSocketService,
                private lightState: LightState) {
        this.socket = websocketService.connect('ws://' + document.location.host + '/ws');
    }

    ngOnInit() {
        this.socket.subscribe(
            (message) => {
                this.message = message.data;

                const channels: string[] = this.message.split(';');

                try {
                    if(channels.length > 0 && !this.lightState.isDragging) {
                        channels.map((channel) => {
                            const channelData = channel.split(',');
                            console.log(channelData);
                            this.lightState.channels[+channelData[0]][+channelData[1]] = +channelData[2];
                        });
                    } else {
                        console.log("skip");
                    }
                } catch (e) {
                    console.info(e);
                }
            }
        );
        /*
         this.launchCounter();
         */
    }

    /*
     private launchCounter() {
     if (this.counterSubscription) {
     this.counterSubscription.unsubscribe();
     }

     let counter = Observable.interval(1000);

     this.counterSubscription = counter.subscribe(
     (num) => {
     this.sentMessage = 'Websocket Message ' + num;
     this.socket.next(this.sentMessage);
     }
     );
     }
     */
}

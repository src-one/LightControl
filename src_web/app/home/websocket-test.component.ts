import {Component, OnInit} from '@angular/core';
import {Subject} from 'rxjs/Rx';
import {WebSocketService} from '../shared/service/websocket.service';
import {LightState} from './light.state';
import {Channel} from './channel.model';
import {ChannelDto} from './channel.dto';

@Component({
    selector: 'websocket-test',
    template: `
        <!--
        <div class="container">
            websocket component loaded...
            <div>Sent Message: {{ sentMessage }}</div>
            <div>Recv Message: {{ message }}</div>
        </div>
        -->
    `,
})
export class WebsocketTestComponent implements OnInit {
    public sentMessage: string = '';

    private socket: Subject<any>;
    //private counterSubscription: Subscription;
    private message: string;

    constructor(websocketService: WebSocketService,
                private lightState: LightState) {
        this.socket = websocketService.connect('ws://' + document.location.host + '/ws');
    }

    public ngOnInit() {
        this.socket.subscribe(
            (message) => {
                if (this.lightState.isDragging) {
                    return;
                }

                this.message = message.data;

                const channels: string[] = this.message.slice(0, -1).split(';');

                try {
                    if (channels.length > 0 && !this.lightState.isDragging) {
                        channels.map((channel) => {
                            const channelData = channel.split(',');
                            //console.log(channelData);
                            this.lightState.channels[+channelData[0]][+channelData[1]] = new Channel({
                                room: +channelData[0],
                                channel: +channelData[1],
                                value: +channelData[2],
                            } as ChannelDto);
                        });
                    } else {
                        //console.log("skip");
                    }
                } catch (e) {
                    console.info(e);
                }
            },
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

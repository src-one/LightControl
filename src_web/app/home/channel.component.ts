import {ChangeDetectionStrategy, Component, EventEmitter, OnInit} from '@angular/core';
import {ObservableHandler} from '../shared/core/observable-handler';
import {Channel} from './channel.model';
import {ChannelDto} from './channel.dto';
import {ObservablePending} from '../shared/core/observable-pending';
import {Channels} from './channels.model';
import {ChannelsDto} from './channels.dto';
import {LightState} from './light.state';
import {LightService} from '../shared/service/light.service';

@Component({
    selector: 'channel',
    changeDetection: ChangeDetectionStrategy.Default,
    template: `
        <div class="row">
            <div class="col-xs-6 visible-xs-inline-block tab"
                 *ngFor="let room of lightState.channels; let roomNum = index"
                 [class.tab--enabled]="currentTab == roomNum"
                 (click)="currentTab = roomNum">
                Room {{ roomNum + 1 }}
            </div>
        </div>

        <div class="row" *ngFor="let room of lightState.channels; let roomNum = index"
             [class.hidden-xs]="currentTab != roomNum">
            <div class="col-xs-3 text-center" *ngFor="let channel of room; let channelNum = index">
                <slider
                        [min]="0"
                        [max]="4095"
                        [value]="channel"
                        (valueChange)="setChannel($event, roomNum, channelNum)"
                        (onDrag)="setChannel($event, roomNum, channelNum)"
                        (onInteract)="onInteract($event)">
                </slider>
            </div>
        </div>

        <div class="row">
            <div class="col-xs-4">
                <button class="btn btn-primary btn-block" (click)="setCinemaMode()">Cinema</button>
            </div>
            <div class="col-xs-4">
                <button class="btn btn-info btn-block" (click)="setPartyMode()">Party</button>
            </div>
            <div class="col-xs-4">
                <button class="btn btn-warning btn-block" (click)="setOff()">Off</button>
            </div>
        </div>
    `
})
export class ChannelComponent implements OnInit {
    private setChannelStream = new EventEmitter<ChannelDto>();
    private setChannelsStream = new EventEmitter<ChannelsDto>();
    public stream: ObservablePending<Channel>;
    public stream2: ObservablePending<Channels>;
    public statusStream: ObservablePending<any>;
    public currentTab: number = 0;

    constructor(private lightService: LightService,
                public lightState: LightState) {
    }

    public ngOnInit() {
        ObservableHandler.from<Channel>(this.setChannelStream, this)
            .setApi(data => this.lightService.setChannel(data.room, data.channel, data.value))
            .onPending(pending => this.stream = pending)
            .subscribe();

        ObservableHandler.from<Channels>(this.setChannelsStream, this)
            .setApi(data => this.lightService.setChannels(data.channels))
            .onPending(pending => this.stream2 = pending)
            .subscribe();
        /*
        ObservableHandler.take<Channels>(null, this)
            .setApi(data => this.lightService.setChannels(data.channels))
            .onPending(pending => this.stream2 = pending)
            .subscribe();
         */
        this.getChannelData();
    }

    public onInteract(value: boolean) {
        this.lightState.isDragging = value;
    }

    public setChannel(value: number, room: number, channel: number) {
        //console.log(room, channel, value);
        //const _value: number = Math.round(this.filter.map(value, 0, 200, 0, 4095));
        //this.lightState.channels[room + '.' + channel] = new Channel({ room: room, channel: channel, value: _value });

        this.setChannelStream.emit({room, channel, value: value});
        //this.setChannelStream.emit({room, channel, value: _value});
    }

    public setCinemaMode() {
        const channels: Channel[] = [
            new Channel({room: 1, channel: 1, value: 100}),
            new Channel({room: 1, channel: 2, value: 512}),
            new Channel({room: 1, channel: 3, value: 0}),
            new Channel({room: 1, channel: 4, value: 512}),
            new Channel({room: 2, channel: 1, value: 0}),
            new Channel({room: 2, channel: 2, value: 0}),
            new Channel({room: 2, channel: 3, value: 0}),
            new Channel({room: 2, channel: 4, value: 0})
        ];

        this.setChannelsStream.emit({channels});
    }

    public setPartyMode() {
        const channels: Channel[] = [
            new Channel({room: 1, channel: 1, value: 2048}),
            new Channel({room: 1, channel: 2, value: 0}),
            new Channel({room: 1, channel: 3, value: 0}),
            new Channel({room: 1, channel: 4, value: 0}),
            new Channel({room: 2, channel: 1, value: 2048}),
            new Channel({room: 2, channel: 2, value: 0}),
            new Channel({room: 2, channel: 3, value: 0}),
            new Channel({room: 2, channel: 4, value: 0})
        ];

        this.setChannelsStream.emit({channels});
    }

    public setOff() {
        const channels: Channel[] = [
            new Channel({room: 1, channel: 1, value: 0}),
            new Channel({room: 1, channel: 2, value: 0}),
            new Channel({room: 1, channel: 3, value: 0}),
            new Channel({room: 1, channel: 4, value: 0}),
            new Channel({room: 2, channel: 1, value: 0}),
            new Channel({room: 2, channel: 2, value: 0}),
            new Channel({room: 2, channel: 3, value: 0}),
            new Channel({room: 2, channel: 4, value: 0})
        ];

        this.setChannelsStream.emit({channels});
    }

    private getChannelData(): void {
        ObservableHandler.take<any>(null, this)
            .setApi(() => this.lightService.getStatus())
            .onPending((stream) => this.statusStream = stream)
            .subscribe((message) => {
                console.log(message);

                const channels: string[] = message.data.split(';');

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
            });
    }
}

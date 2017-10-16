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
                        [value]="channel.value"
                        (valueChange)="setChannel($event, channel.room, channel.channel)"
                        (onDrag)="setChannel($event, channel.room, channel.channel)"
                        (onInteract)="onInteract($event)">
                </slider>
            </div>
        </div>

        <div class="row">
            <div class="col-xs-3">
                <button class="btn btn-primary btn-block" (click)="setCinemaMode()">Cinema</button>
            </div>
            <div class="col-xs-3">
                <button class="btn btn-info btn-block" (click)="setPartyMode()">Party</button>
            </div>
            <div class="col-xs-3">
                <button class="btn btn-success btn-block" (click)="setFancyMode()">Fancy</button>
            </div>
            <div class="col-xs-3">
                <button class="btn btn-warning btn-block" (click)="setOff()">Off</button>
            </div>
        </div>
    `,
})
export class ChannelComponent implements OnInit {
    public currentTab: number = 0;

    private setChannelStream = new EventEmitter<ChannelDto>();
    private setChannelsStream = new EventEmitter<ChannelsDto>();
    private stream: ObservablePending<Channel>;
    private stream2: ObservablePending<Channels>;
    private statusStream: ObservablePending<any>;

    constructor(private lightService: LightService,
                public lightState: LightState) {
    }

    public ngOnInit() {
        ObservableHandler.from<Channel>(this.setChannelStream, this)
            .setApi((data) => this.lightService.setChannel(data.room, data.channel, data.value))
            .onPending((pending) => this.stream = pending)
            .subscribe();

        ObservableHandler.from<Channels>(this.setChannelsStream, this)
            .setApi((data) => this.lightService.setChannels(data.channels))
            .onPending((pending) => this.stream2 = pending)
            .subscribe();
        this.getChannelData();
    }

    public onInteract(value: boolean) {
        this.lightState.isDragging = value;
    }

    public setChannel(value: number, room: number, channel: number) {
        this.setChannelStream.emit({room, channel, value: value});
    }

    public setCinemaMode() {
        const channels: Channel[] = [
            new Channel({room: 0, channel: 0, value: 100}),
            new Channel({room: 0, channel: 1, value: 0}),
            new Channel({room: 0, channel: 2, value: 0}),
            new Channel({room: 0, channel: 3, value: 0}),
            new Channel({room: 1, channel: 0, value: 100}),
            new Channel({room: 1, channel: 1, value: 0}),
            new Channel({room: 1, channel: 2, value: 0}),
            new Channel({room: 1, channel: 3, value: 0}),
        ];

        this.setChannelsStream.emit({channels});
    }

    public setPartyMode() {
        const channels: Channel[] = [
            new Channel({room: 0, channel: 0, value: 0}),
            new Channel({room: 0, channel: 1, value: 250}),
            new Channel({room: 0, channel: 2, value: 0}),
            new Channel({room: 0, channel: 3, value: 150}),
            new Channel({room: 1, channel: 0, value: 0}),
            new Channel({room: 1, channel: 1, value: 250}),
            new Channel({room: 1, channel: 2, value: 0}),
            new Channel({room: 1, channel: 3, value: 150}),
        ];

        this.setChannelsStream.emit({channels});
    }

    public setFancyMode() {
        const channels: Channel[] = [
            new Channel({room: 0, channel: 0, value: 0}),
            new Channel({room: 0, channel: 1, value: 564}),
            new Channel({room: 0, channel: 2, value: 0}),
            new Channel({room: 0, channel: 3, value: 4095}),
            new Channel({room: 1, channel: 0, value: 0}),
            new Channel({room: 1, channel: 1, value: 4095}),
            new Channel({room: 1, channel: 2, value: 138}),
            new Channel({room: 1, channel: 3, value: 1967}),
        ];

        this.setChannelsStream.emit({channels});
    }

    public setOff() {
        const channels: Channel[] = [
            new Channel({room: 0, channel: 0, value: 0}),
            new Channel({room: 0, channel: 1, value: 0}),
            new Channel({room: 0, channel: 2, value: 0}),
            new Channel({room: 0, channel: 3, value: 0}),
            new Channel({room: 1, channel: 0, value: 0}),
            new Channel({room: 1, channel: 1, value: 0}),
            new Channel({room: 1, channel: 2, value: 0}),
            new Channel({room: 1, channel: 3, value: 0}),
        ];

        this.setChannelsStream.emit({channels});
    }

    private getChannelData(): void {
        ObservableHandler.take<any>(null, this)
            .setApi(() => this.lightService.getStatus())
            .onPending((stream) => this.statusStream = stream)
            .subscribe((message) => {
                const channels: string[] = message.data.slice(0, -1).split(';');

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
            });
    }
}

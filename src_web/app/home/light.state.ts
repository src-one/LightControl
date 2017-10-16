import {Injectable} from '@angular/core';
import {Channel} from './channel.model';

@Injectable()
export class LightState {
    public isDragging: boolean = false;

    public channels: Channel[][] = [
        [new Channel(), new Channel(), new Channel(), new Channel()],
        [new Channel(), new Channel(), new Channel(), new Channel()],
    ];
}

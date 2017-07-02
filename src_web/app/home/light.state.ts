import {Injectable} from '@angular/core';

@Injectable()
export class LightState {
    public isDragging: boolean = false;

    public channels: number[][] = [
        [0, 0, 0, 0],
        [0, 0, 0, 0]
    ];
}

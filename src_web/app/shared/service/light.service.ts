import {Injectable} from '@angular/core';
import {Observable} from 'rxjs/Observable';
import 'rxjs/add/operator/catch';
import {Http} from '@angular/http';
import {Channel} from '../../home/channel.model';
import {Channels} from '../../home/channels.model';

@Injectable()
export class LightService {
    constructor(private http: Http) {
    }

    public getStatus(): Observable<Channel[]> {
        return this.http.get('/api/channels')
            .map(response => response.json());
    }

    public setChannel(room: number, channel: number, value: number): Observable<Channel> {
        //const payload: string = room + '.' + channel + '.' + value;
        const payload = {room: room, channel: channel, value: value};
        const params = {};

        return this.http.post('/api/channel', [payload], params)
            .map(response => new Channel(response.json()));
    }

    public setChannels(channels: Channel[]): Observable<Channels> {
        /*
         const payload: string = channels.map((entry) => {
         return entry.room + '.' + entry.channel + '.' + entry.value;
         }).join(';');
         */

        const payload = channels;
        const params = {};

        //console.log(payload);

        return this.http.post('/api/channel', payload, params)
            .map(response => new Channels(response.json()));
    }
}

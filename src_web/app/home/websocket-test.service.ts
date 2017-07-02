import {Injectable} from '@angular/core';
import {Subject} from 'rxjs/Subject';
import {WebSocketService} from '../shared/service/websocket.service';
import 'rxjs/add/operator/map';
import 'rxjs/add/operator/filter';

const CHAT_URL = 'ws://' + document.location.host + ':3005';
const DATA_URL = 'ws://' + document.location.host + ':3006';

export interface Message {
    author: string,
    message: string,
    newDate?: string
}

@Injectable()
export class WebsocketTestService {
    public messages: Subject<Message> = new Subject<Message>();
    public randomData: Subject<number> = new Subject<number>();

    constructor(public service: WebSocketService) {
        this.messages = <Subject<Message>>service
            .connect(CHAT_URL)
            .map((response: MessageEvent): Message => {
                let data = JSON.parse(response.data);
                return {
                    author: data.author,
                    message: data.message,
                    newDate: data.newDate
                }
            });

        this.randomData = <Subject<number>>service
            .connectData(DATA_URL)
            .map((response: any): number => {
                return response.data;
            });
    }
}

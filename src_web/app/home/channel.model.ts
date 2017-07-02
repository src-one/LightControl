import {ChannelDto} from './channel.dto';

export class Channel {
    public room: number;
    public channel: number;
    public value: number;

    constructor(channel: ChannelDto) {
        this.room = channel.room;
        this.channel = channel.channel;
        this.value = channel.value;
    }
}

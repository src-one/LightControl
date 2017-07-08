import {ChannelDto} from './channel.dto';

export class Channel {
    public room: number = 0;
    public channel: number = 0;
    public value: number = 0;

    constructor(channel?: ChannelDto) {
        if (!channel) {
            return;
        }

        this.room = channel.room;
        this.channel = channel.channel;
        this.value = channel.value;
    }
}

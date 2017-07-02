import {ChannelsDto} from './channels.dto';
import {Channel} from './channel.model';

export class Channels {
    public channels: Channel[];

    constructor(channels: ChannelsDto) {
        this.channels = channels.channels.map((channel) => new Channel(channel));
    }
}

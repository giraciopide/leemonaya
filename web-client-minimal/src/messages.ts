import { Option } from 'prelude-ts';

interface StationReadingsMsg {
    stationId: string;
    temperature: number;
    humidity: number;
}

function validateStationReadingMsg(obj: any): Option<StationReadingsMsg> {
    return Option.ofNullable(obj)
        .filter(o => o.hasOwnProperty('stationId'))
        .filter(o => o.hasOwnProperty('temperature'))
        .filter(o => o.hasOwnProperty('humidity'))
        .map(o => o as StationReadingsMsg);
}

interface StationFeed extends StationReadingsMsg {
    id: string;
    tstamp: number; // millis epoch
}

interface StationFeeds {
    feeds: StationFeed[];
}

export { StationReadingsMsg, StationFeed, StationFeeds, validateStationReadingMsg as isValidStationReadingMsg };
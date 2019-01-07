import { StationFeeds, StationFeed } from "./messages";
import { ChartPoint } from "chart.js";
import { Option } from 'prelude-ts';

const BACKEND_URL = 'http://leemonaya.tilaa.cloud:5000'; // set by configure script in the root folder.

interface StationFeedSerie {
    label: string,
    values: ChartPoint[]
}

function emptySerie(label: string): StationFeedSerie {
    return { label: label, values: [] }
}

function toStationFeedSeries(sf: StationFeeds): StationFeedSerie[] {
    const seriesByLabel = new Map<string, StationFeedSerie>();

    // adds to the map in a safe way.
    let add = function(label: string, tstamp: Date, value: number) : void {
        const serie = Option.ofNullable(seriesByLabel.get(label)).getOrCall(() => emptySerie(label));
        serie.values.push({ x: tstamp, y: value });
        seriesByLabel.set(label, serie)
    }
    
    // categorize feeds by station/measurement
    sf.feeds.forEach(feed => {
        const tstamp = new Date(feed.tstamp)
        add(feed.stationId + '/temperature C°', tstamp, feed.temperature)
        add(feed.stationId + '/humidity %', tstamp, feed.humidity)
    })
    
    const out: StationFeedSerie[] = []
    seriesByLabel.forEach((v, k) => out.push(v))
    return out;
}

function loadSince(fromTime: number): Promise<StationFeedSerie[]> {
    return fetch(`${BACKEND_URL}/data/from/${fromTime}`)
        .then(response => response.json())
        .then(body => body as StationFeeds)
        .then(toStationFeedSeries)
}

// mock version
function _loadSince(fromTime: number): Promise<StationFeedSerie[]> {

    let randomPoint = function(dateOffset: number): ChartPoint {
        return {
            x: new Date(new Date().getTime() + dateOffset),
            y: Math.random() * 35
        }
    }

    let randomPoints = function(count: number): ChartPoint[] {
        const out = new Array(count);
        for (let i = 0; i < out.length; ++i) {
            out[i] = randomPoint(i * 10); // 10 millis from each other
        }
        return out;
    }

    const out = [
        {
            label: 'fakeSource1/temperature C°',
            values: randomPoints(5)
        },
        {
            label: 'fakeSource1/humidity %',
            values: randomPoints(5)
        }
    ];

    console.log(JSON.stringify(out));
    return Promise.resolve(out);
}

export { StationFeedSerie, loadSince as fetchStationDataSince }
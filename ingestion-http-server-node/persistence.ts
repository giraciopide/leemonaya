import { StationReadingsMsg, StationFeed } from "./messages";
import BetterSqlite3 from 'better-sqlite3';
import Database from 'better-sqlite3';

export interface PersistenceService {
    store(payload: StationReadingsMsg): StationFeed;
    load(from: number, to: number): StationFeed[];
    loadLatest(from: number): StationFeed[];
}

interface Row {
    rowId: any;
    stationId: string,
    tstamp: number,
    payload: string
}

function toStationFeed(row: Row): StationFeed {
    const readings = JSON.parse(row.payload) as StationReadingsMsg;
    return {
        id: row.rowId.toString(),
        stationId: row.stationId,
        tstamp: row.tstamp,
        humidity: readings.humidity,
        temperature: readings.temperature
    }
}

class Sqlite3PersistenceService implements PersistenceService {

    private readonly db: BetterSqlite3.Database;
    
    constructor(fileName: string) {
        this.db = new Database(fileName);
        const stmt = this.db.prepare(`
            CREATE TABLE IF NOT EXISTS station_readings
                (station_id TEXT,
                tstamp INTEGER,
                payload TEXT)
        `);
        stmt.run();
    }

    store(msg: StationReadingsMsg): StationFeed {
        const statement = this.db.prepare(`
            INSERT INTO station_readings (station_id, tstamp, payload) VALUES (?, ?, ?)
        `)
        const tstamp = new Date().getTime();
        const result = statement.run(msg.stationId, tstamp, JSON.stringify(msg));
        return {
            id: result.lastInsertRowid.toString(),
            stationId: msg.stationId,
            humidity: msg.humidity,
            temperature: msg.temperature,
            tstamp: tstamp
        }
    }

    load(from: number, to: number): StationFeed[] {
        const statement = this.db.prepare(`
            SELECT * FROM station_readings WHERE tstamp > ? AND tstamp < ?
        `)
        return (statement.all(from, to) as Row[]).map(toStationFeed);
    }

    loadLatest(from: number): StationFeed[] {
        const statement = this.db.prepare(`
            SELECT * FROM station_readings WHERE tstamp > ?
        `)
        return (statement.all(from) as Row[]).map(toStationFeed);
    }
}

export class PersistenceServiceFactory {
    public static get(): PersistenceService {
        return new Sqlite3PersistenceService('leemonaya.sqlite3'); // TODO make configurable
    }
}
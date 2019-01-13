import { Option } from "prelude-ts";

/**
 * groupByCount([1, 2, 3, 4, 5], 2) returns [[1, 2], [3, 4], [5]]
 */
export function groupByCount<T>(items: T[], groupSize: number): T[][] {
    const out: T[][] = [];
    let bucket: T[] = [];
    for (let i = 0; i < items.length; ++i) {
        if ((i % groupSize) == 0) {
            bucket = [];
            out.push(bucket);
        }
        bucket.push(items[i]);
    }
    return out;
}

/**
 * Group the given array by a grouping function
 */
export function groupBy<T, G>(ts: T[], fn: (t: T) => G): T[][] {
    const groups = new Map<String, T[]>();
    for (let t of ts) {
        const groupKey = JSON.stringify(fn(t))
        const group = groups.get(groupKey)
        if (group) {
            group.push(t)
        } else {
            groups.set(groupKey, [t])
        }
    }
    
    return [ ...groups.values() ];
}


export function asNumber(obj: any): number {
    if (typeof(obj) !== 'string') {
        throw new Error('Cannot convert a non-string to a number');
    }
    return Option.ofNullable(parseInt(obj as string))
        .filter(num => !Number.isNaN(num))
        .getOrThrow('Not a valid number');
}


export function stringToUint8Array(s: String): Uint8Array {
    let ua = new Uint8Array(s.length);
    for (let i = 0; i < s.length; i++) {
        ua[i] = s.charCodeAt(i);
    }
    return ua;
}


export function Uint8ArrayToString(ua: Uint8Array): string {
    let s = '';
    for (let i = 0; i < ua.length; i++) {
        s += String.fromCharCode(ua[i]);
    }
    return s;
}


export function nearestTick(value: number, tick: number) {
    return tick * Math.round(value / tick);
}
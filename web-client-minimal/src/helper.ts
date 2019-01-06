import { Option } from 'prelude-ts';

function isCanvas(element: HTMLElement): boolean {
    if (element instanceof HTMLCanvasElement) {
        return true;
    } 
    return false;
}

export function assertIsCanvas(element: HTMLElement): void {
    if (!isCanvas(element)) {
        throw new Error('Is not an html canvas element');
    }
}

//
// Old skool, broken as fuck and proudly so. 
// I hate dates, milliseconds since 1970 are a very nice way to represent them.
//

export function nowUTC(): number {
    return new Date().getTime();
}

export function secondsAsMillis(count: number) {
    return 1000 * count;
}

export function minutesAsMillis(count: number) {
    return count * 60 * secondsAsMillis(1);
}

export function hoursAsMillis(count: number) {
    return count * 60 * minutesAsMillis(1);
}

export function daysAsMillis(count: number) {
    return count * 24 * hoursAsMillis(1);
}

export class Timer {
    private start: number;
    private lapStart: number;

    constructor() {
        this.start = nowUTC();
        this.lapStart = this.start;
    }

    public elapsed(): number {
        return nowUTC() - this.start;
    }

    public getCurrentLapStart(): number {
        return this.lapStart;
    }

    public lap(): number {
        const now = nowUTC()
        const out = now - this.lapStart;
        this.lapStart = now;
        return out;
    }
}

/**
 * Executes fn at the next event loop.
 */
export function later(fn: () => void) {
    setTimeout(fn, 0);
}
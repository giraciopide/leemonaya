/*
 * Copyright (c) 2018 Marco Nicolini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

import { Option, stringHashCode } from 'prelude-ts';
import Chart from 'chart.js';
import { assertIsCanvas, later, nowUTC, daysAsMillis as days, secondsAsMillis, Timer } from './helper';
import { LeemonayaChart } from './LeemonayaChartData';
import { fetchStationDataSince } from './backend';

function main() {
    // display data from the last 2 days.
    const twoDaysAgo = nowUTC() - days(2);

    // fetch the initial data, and create the graph
    const chart: Promise<LeemonayaChart> = fetchStationDataSince(twoDaysAgo)
        .then(initialData => Option.ofNullable(document.getElementById('graph'))
            .ifSome(assertIsCanvas)
            .map(el => el as HTMLCanvasElement)
            .map(canvas => new LeemonayaChart(canvas, initialData))
            .getOrThrow())

    // every n seconds, poll the server and update the graph.
    const timer = new Timer();
    const pollInterval = secondsAsMillis(5);
    setInterval(() => {
        chart.then(c => {
            return Promise.all([Promise.resolve(c), fetchStationDataSince(timer.getCurrentLapStart())])
        }).then(res => {
            const newSeries = res[1];
            const chart = res[0];
            chart.pushAll(newSeries);
            timer.lap();
        })
    }, pollInterval);
}

// let's go.
main();
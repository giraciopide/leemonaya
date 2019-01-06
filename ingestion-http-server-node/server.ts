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

import express, {Express, Request} from 'express'
import bodyParser from 'body-parser'
import http from 'http'
import { PersistenceServiceFactory } from './persistence'
import { isValidStationReadingMsg } from './messages'
import { HMAC } from "fast-sha256"
import { Option } from 'prelude-ts';

/**
 * Configuration
 */

const port: number = 3000
const hostname: string = '0.0.0.0'
const disableAuth = false
const hmacKey = stringToUint8Array('12ekjnrl1kjnrlqsakndvqwjern1pe2on1lkndv1ldnvcasdcasdc1jn');

/**
 * Real stuff starts here
 */

const app: Express = express()
const persistence = PersistenceServiceFactory.get();

app.use('/station-data', bodyParser.json({
    type: '*/*',
    limit: '10kb',
    verify: (req, res, buf, encoding) => {
        if (disableAuth) return;
        const rawBody = new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength);
        getAuthorizationHeader(req.headers)
            .map(authorizationHeader => hmacSignatureMatches(rawBody, hmacKey, authorizationHeader))
            .filter(valid => valid)
            .ifNone(() => {
                const err = new Error('Invalid or missing Authorization header');
                (err as any).statusCode = 401;
                (err as any).statusMessage = 'Invalid or missing Authorization header';
                throw err;
            })
     }
}));

app.post('/station-data', (req, res, next) => {
    isValidStationReadingMsg(req.body)
        .map(msg => persistence.store(msg)) // persists and returns a feed.
        .ifSome(feed => console.log(`station feed [${JSON.stringify(feed)}]`))
        .getOrThrow('Error while processing station data')
    res.sendStatus(204)
})

app.get('/test/', (req, res) => {
    res.json({message: 'Hello World'})
})

/*
Test with:
curl -i --header 'Authorization: QdI9+pLJ1kVaLQE8zmz/SJoDVFqC+Bk/m1Vbbr5/hPs=' \
        --header "Content-type: application/json" \
        -d '{"stationId":"testbench","temperature":12,"humidity":23}' \
        http://localhost:3000/station-data
*/

app.listen(port, hostname, () => {
    console.log('====================================================')
    console.log(`Leemonaya API server UP at [${hostname}:${port}]`);
    console.log('====================================================')
    console.log('Firmware friendly hmac key:');
    console.log(asFirmwareFriendlyDeclaration(hmacKey));
})

//
// helpers
//

function stringToUint8Array(s: String): Uint8Array {
    let ua = new Uint8Array(s.length);
    for (let i = 0; i < s.length; i++) {
        ua[i] = s.charCodeAt(i);
    }
    return ua;
}

function Uint8ArrayToString(ua: Uint8Array): string {
    let s = '';
    for (let i = 0; i < ua.length; i++) {
        s += String.fromCharCode(ua[i]);
    }
    return s;
}

function asFirmwareFriendlyDeclaration(data: Uint8Array): string {
    return '#define HMAC_KEY_LENGTH ' + data.length + '\nstatic uint8_t hmac_key[HMAC_KEY_LENGTH] = {' + data.join(', ') + '};';
}

function hmacSignatureMatches(data: Uint8Array, key: Uint8Array, base64EncodedSignature: string): boolean {
    const h = new HMAC(key)
    const binaryHash = h.update(data).digest()
    const base64ComputedHash = Buffer.from(binaryHash).toString('base64')
    const valid = base64ComputedHash == base64EncodedSignature;
    if (!valid) {
        console.debug('HMAC SHA256 does not matches');
        console.debug(`server computed hmac_sha256: [${base64ComputedHash}]`)
        console.debug(`client provided hmac_sha256: [${base64EncodedSignature}]`)
    }
    return valid;
}

function getAuthorizationHeader(headers: http.IncomingHttpHeaders): Option<string> {
    const h = headers['authorization'];
    const hh = Array.isArray(h) ? h[0] : h;
    return Option.ofNullable(hh);
}

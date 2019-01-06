import { Option } from 'prelude-ts';

const x: number = 42;
const message: string = `Hello ${x}`;

Option.ofNullable(document.getElementById("root"))
    .ifSome(element => {
        element.innerHTML = message;
    });

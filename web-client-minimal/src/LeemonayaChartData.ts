import { Option } from 'prelude-ts';
import { ChartData, ChartDataSets, ChartPoint, ChartColor } from 'chart.js';
import { StationFeedSerie } from './backend';
import Chart from 'chart.js';

class ColorPalette {
    private i: number;
    private palette: ChartColor[]

    constructor() {
        this.i = 0;
        this.palette = ['#C7375B', '#8694E0', '#3A3A4E', '#E0979E', '#F68E6A'];
    }

    public next(): ChartColor {
        const color = this.palette[this.i];
        this.i++;
        if (this.i >= this.palette.length) {
            this.i = 0;
        }
        return color;
    }
}

export class LeemonayaChart {
    private readonly canvas: HTMLCanvasElement;
    private readonly byLabel: Map<string, ChartDataSets>;
    private readonly series: ChartData;
    private readonly datasets: ChartDataSets[];
    private readonly chart: Chart;
    private readonly palette: ColorPalette;

    constructor(canvas: HTMLCanvasElement, initialdData: StationFeedSerie[]) {
        this.palette = new ColorPalette();
        this.canvas = canvas;
        this.byLabel = new Map<string, ChartDataSets>();
        this.datasets = [];
        initialdData.forEach(i => this.addToDatasets(i, this.datasets));
        this.series = {
            datasets: this.datasets
        };
        this.chart = new Chart(canvas, {
            type: 'line',
            data: this.series,
            options: {
                legend: {
                    position: 'right'
                },
                title: {
                    fontSize: 20,
                    display: true,
                    text: 'Leemonaya!'
                },
                scales: {
                    xAxes: [{
                        type: 'time',
                        time: {
                            unit: 'second'
                        },
                        ticks: {
                          autoSkip: true  
                        }
                    }]
                }
            }
        })
        this.chart.update();
    }

    private push(serie: StationFeedSerie): void {
        this.addToDatasets(serie, this.datasets);
    }

    public pushAll(series: StationFeedSerie[]): void {
        series.forEach(s => this.push(s));
        this.chart.update();
    }

    private addToDatasets(serie: StationFeedSerie, datasets: ChartDataSets[]): void {
        const cds = Option.ofNullable(this.byLabel.get(serie.label))
            .getOrCall(() => {
                const s = this.emptyChartDataSets(serie.label);
                this.byLabel.set(serie.label, s);
                datasets.push(s);
                return s;
            });
        const points = cds.data as ChartPoint[];
        serie.values.forEach(point => points.push(point));
    }

    private emptyChartDataSets(label: string): ChartDataSets {
        return {
            label: label,
            fill: false,
            data: [],
            borderColor: this.palette.next()
        };
    }
}

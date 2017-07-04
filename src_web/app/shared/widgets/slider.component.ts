import {Component, AfterViewInit, ElementRef, Input, Output, EventEmitter, ChangeDetectionStrategy} from '@angular/core';

@Component({
    selector: 'slider',
    changeDetection: ChangeDetectionStrategy.Default,
    template: `
        <div class="slider">
            <div class="slider-track">
                <div class="slider-track__bar" role="progressbar" aria-valuenow="40" aria-valuemin="min" aria-valuemax="max" [style.height]="((value / max * 100)) + '%'"></div>
            </div>
            <input class="slider__control {{sliderClass}}"
                   id="{{ sliderId }}"
                   type="range"
                   [min]="min"
                   [max]="max"
                   [step]="step"
                   [disabled]="disabled"
                   [(ngModel)]="value"
                   (change)="change($event)"
                   (input)="drag($event)"
                   (mouseup)="leave($event)"
                   (mousedown)="interact($event)"
                   (touchend)="touchend($event)"
            />
            <div class="slider__caption">{{ caption || value || 0 }}</div>
        </div>
    `
})

export class SliderComponent implements AfterViewInit {
    @Input() public min: number = 0;
    @Input() public max: number = 4095;
    @Input() public step: number = 1;
    @Input() public disabled: boolean = false;
    @Input() public value: number = 0;
    @Input() public caption: number|string;
    @Input() public sliderClass: string;
    @Output() public onDrag = new EventEmitter<number>();
    @Output() public onChange = new EventEmitter<number>();
    @Output() public valueChange = new EventEmitter<number>();
    @Output() public onInteract = new EventEmitter<boolean>();

    public sliderId: string = getUniqueId('slider_');
    private $slider: any;
    private interacting: boolean = false;
    // private $sliderTrack: any;

    constructor(public elementRef: ElementRef) {
    }

    public ngAfterViewInit(): void {
        this.$slider = $('#' + this.sliderId);
        // this.$sliderTrack = $("input[type='range']::-webkit-slider-runnable-track");
    }

    public drag(): void {
        if(this.interacting == false) {
            this.onInteract.emit(true);
            this.interacting = true;
        }

        this.onDrag.emit(+this.value);

        //console.log('dragging', this.interacting);
    }

    public leave(): void {
        this.interacting = false;
        this.onInteract.emit(false);
        this.onDrag.emit(+this.value);

        //console.log('leave', this.interacting);
    }

    public interact(): void {
        this.interacting = true;
        this.onInteract.emit(true);

        //console.log('interact', this.interacting);
    }

    public touchend(): void {
        this.interacting = false;
        this.onInteract.emit(false);

        //console.log('touch end', this.interacting);
    }

    public getPercentage(): number {
        return Math.round((this.value / 4095 * 100));
    }

    public change(): void {
        this.interacting = false;
        this.onInteract.emit(false);
        this.onChange.emit(+this.value);
        this.valueChange.emit(+this.value);
        // this.valueChange.emit(+this.value);
        // this.$sliderTrack.css({ 'background-size': '20% 100%' });
        // console.log(this.$sliderTrack, Math.round((this.value / 255 * 100)));

        //console.log('change', this.interacting);
    }
}

let uniqueId: number = 0;

function getUniqueId(prefix: string): string {
    const id: number = ++uniqueId;

    return prefix + id;
}

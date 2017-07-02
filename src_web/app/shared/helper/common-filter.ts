export class Filter {
    public map(value: number, inMin: number, inMax: number, outMin: number, outMax: number): number {
        return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
    }
}

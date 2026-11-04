export namespace main {
	
	export class Status {
	    variant: string;
	    dataRoot: string;
	    root: string;
	
	    static createFrom(source: any = {}) {
	        return new Status(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.variant = source["variant"];
	        this.dataRoot = source["dataRoot"];
	        this.root = source["root"];
	    }
	}

}

export namespace tebox {
	
	export class DeviceProfile {
	    latitude: number;
	    longitude: number;
	    country: string;
	    operatorNumeric: string;
	    operatorName: string;
	    cellId: string;
	    phoneNumber: string;
	    root: boolean;
	
	    static createFrom(source: any = {}) {
	        return new DeviceProfile(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.latitude = source["latitude"];
	        this.longitude = source["longitude"];
	        this.country = source["country"];
	        this.operatorNumeric = source["operatorNumeric"];
	        this.operatorName = source["operatorName"];
	        this.cellId = source["cellId"];
	        this.phoneNumber = source["phoneNumber"];
	        this.root = source["root"];
	    }
	}
	export class Instance {
	    id: string;
	    name: string;
	    variant: string;
	    dir: string;
	    adbPort: number;
	    pid: number;
	    running: boolean;
	    profile: DeviceProfile;
	
	    static createFrom(source: any = {}) {
	        return new Instance(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.id = source["id"];
	        this.name = source["name"];
	        this.variant = source["variant"];
	        this.dir = source["dir"];
	        this.adbPort = source["adbPort"];
	        this.pid = source["pid"];
	        this.running = source["running"];
	        this.profile = this.convertValues(source["profile"], DeviceProfile);
	    }
	
		convertValues(a: any, classs: any, asMap: boolean = false): any {
		    if (!a) {
		        return a;
		    }
		    if (a.slice && a.map) {
		        return (a as any[]).map(elem => this.convertValues(elem, classs));
		    } else if ("object" === typeof a) {
		        if (asMap) {
		            for (const key of Object.keys(a)) {
		                a[key] = new classs(a[key]);
		            }
		            return a;
		        }
		        return new classs(a);
		    }
		    return a;
		}
	}

}


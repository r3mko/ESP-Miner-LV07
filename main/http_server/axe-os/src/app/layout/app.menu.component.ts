import { Component, OnInit } from '@angular/core';
import { Observable, shareReplay } from 'rxjs';
import { ToastrService } from 'ngx-toastr';
import { SystemService } from '../services/system.service';
import { LayoutService } from './service/app.layout.service';
import { ISystemInfo } from 'src/models/ISystemInfo';

@Component({
    selector: 'app-menu',
    templateUrl: './app.menu.component.html'
})
export class AppMenuComponent implements OnInit {

    public info$!: Observable<ISystemInfo>;

    model: any[] = [];

    constructor(public layoutService: LayoutService,
        private systemService: SystemService,
        private toastr: ToastrService
    ) {
        this.info$ = this.systemService.getInfo().pipe(shareReplay({refCount: true, bufferSize: 1}))
    }

    ngOnInit() {
        this.model = [
            {
                label: 'Menu',
                items: [
                    { label: 'Dashboard', icon: 'pi pi-fw pi-home', routerLink: ['/'] },
                    { label: 'Swarm', icon: 'pi pi-fw pi-sitemap', routerLink: ['swarm'] },
                    { label: 'Logs', icon: 'pi pi-fw pi-list', routerLink: ['logs'] },
                    { label: 'System', icon: 'pi pi-fw pi-wave-pulse', routerLink: ['system'] },
                    { separator: true },

                    { label: 'Pool', icon: 'pi pi-fw pi-server', routerLink: ['pool'] },
                    { label: 'Network', icon: 'pi pi-fw pi-wifi', routerLink: ['network'] },
                    { label: 'Theme', icon: 'pi pi-fw pi-palette', routerLink: ['design'] },
                    { label: 'Settings', icon: 'pi pi-fw pi-cog', routerLink: ['settings'] },
                    { label: 'Update', icon: 'pi pi-fw pi-sync', routerLink: ['update'] },
                    { separator: true },

                    { label: 'Whitepaper', icon: 'pi pi-fw pi-bitcoin', command: () => window.open('/bitcoin.pdf', '_blank') },
                ]
            }
        ];
    }

    public restart() {
        this.systemService.restart().subscribe(res => {});
        this.toastr.success('Success!', 'Bitaxe restarted');
    }

    public isMobile() {
        return !window.matchMedia("(min-width: 991px)").matches;
    }
}

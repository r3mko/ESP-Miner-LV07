import { ComponentFixture, TestBed } from '@angular/core/testing';

import { UpdateComponent } from './update.component';
import { ModalComponent } from '../modal/modal.component';
import { FileUpload, FileUploadHandlerEvent, FileUploadModule } from 'primeng/fileupload';
import { CheckboxModule } from 'primeng/checkbox';
import { ProgressBarModule } from 'primeng/progressbar';
import { ButtonModule } from 'primeng/button';
import { HttpEventType, provideHttpClient } from '@angular/common/http';
import { provideToastr, ToastrService } from 'ngx-toastr';
import { SystemApiService } from 'src/app/services/system.service';
import { of } from 'rxjs';

describe('UpdateComponent', () => {
  let component: UpdateComponent;
  let fixture: ComponentFixture<UpdateComponent>;
  let systemService: SystemApiService;
  let toastrService: ToastrService;

  const createUploadEvent = (filename: string): FileUploadHandlerEvent => ({
    files: [new File(['firmware'], filename)],
  } as FileUploadHandlerEvent);

  beforeEach(() => {
    TestBed.configureTestingModule({
      declarations: [UpdateComponent, ModalComponent],
      imports: [FileUploadModule, CheckboxModule, ButtonModule, ProgressBarModule],
      providers: [provideHttpClient(), provideToastr()]
    });
    fixture = TestBed.createComponent(UpdateComponent);
    component = fixture.componentInstance;
    systemService = TestBed.inject(SystemApiService);
    toastrService = TestBed.inject(ToastrService);
    fixture.detectChanges();
    component.firmwareUpload = jasmine.createSpyObj<FileUpload>('FileUpload', ['clear']);
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });

  it('should map board 312 to the MCN16R2 firmware', () => {
    expect(component.getExpectedFirmwareFilename('312')).toBe('esp-miner-mcn16r2.bin');
    expect(component.getExpectedFirmwareFilename(' 312 ')).toBe('esp-miner-mcn16r2.bin');
  });

  it('should map every other board version to the MCN16R8 firmware', () => {
    ['302', '303', '600', 'unknown', '', undefined].forEach(boardVersion => {
      expect(component.getExpectedFirmwareFilename(boardVersion)).withContext(String(boardVersion)).toBe('esp-miner-mcn16r8.bin');
    });
  });

  it('should expose only the board-compatible module firmware as a release asset', () => {
    expect(component.isFirmwareReleaseAsset('esp-miner-mcn16r2.bin', '312')).toBeTrue();
    expect(component.isFirmwareReleaseAsset('esp-miner-mcn16r8.bin', '312')).toBeFalse();
    expect(component.isFirmwareReleaseAsset('esp-miner-mcn16r8.bin', '302')).toBeTrue();
    expect(component.isFirmwareReleaseAsset('esp-miner-mcn16r2.bin', '302')).toBeFalse();
    expect(component.isFirmwareReleaseAsset('esp-miner.bin', '312')).toBeFalse();
    expect(component.isFirmwareReleaseAsset('esp-miner.bin', '302')).toBeFalse();
  });

  it('should detect when a release lacks compatible firmware', () => {
    const assets = [
      { name: 'esp-miner.bin' },
      { name: 'esp-miner-mcn16r8.bin' },
      { name: 'www.bin' },
    ];

    expect(component.hasCompatibleFirmwareAsset(assets, '302')).toBeTrue();
    expect(component.hasCompatibleFirmwareAsset(assets, '312')).toBeFalse();
  });

  it('should upload matching module firmware immediately', () => {
    const otaSpy = spyOn(systemService, 'performOTAUpdate').and.returnValue(of({ type: HttpEventType.Response, ok: true } as any));
    const mcn16r2Event = createUploadEvent('esp-miner-mcn16r2.bin');
    const mcn16r8Event = createUploadEvent('esp-miner-mcn16r8.bin');

    component.otaUpdate(mcn16r2Event, '312');
    component.otaUpdate(mcn16r8Event, '302');

    expect(otaSpy).toHaveBeenCalledWith(mcn16r2Event.files[0]);
    expect(otaSpy).toHaveBeenCalledWith(mcn16r8Event.files[0]);
    expect(component.pendingFirmwareFile).toBeNull();
  });

  it('should reject mismatched and non-firmware files', () => {
    const otaSpy = spyOn(systemService, 'performOTAUpdate');
    const errorSpy = spyOn(toastrService, 'error');

    component.otaUpdate(createUploadEvent('esp-miner-mcn16r8.bin'), '312');
    component.otaUpdate(createUploadEvent('esp-miner-mcn16r2.bin'), '302');
    component.otaUpdate(createUploadEvent('esp-miner-factory-lv07.bin'), '312');
    component.otaUpdate(createUploadEvent('www.bin'), '312');

    expect(otaSpy).not.toHaveBeenCalled();
    expect(errorSpy).toHaveBeenCalledTimes(4);
  });

  it('should require confirmation before uploading legacy firmware', () => {
    const otaSpy = spyOn(systemService, 'performOTAUpdate').and.returnValue(of({ type: HttpEventType.Response, ok: true } as any));
    const event = createUploadEvent('esp-miner.bin');

    component.otaUpdate(event, '312');

    expect(otaSpy).not.toHaveBeenCalled();
    expect(component.pendingFirmwareFile).toBe(event.files[0]);
    expect(component.firmwareCompatibilityModal?.isVisible).toBeTrue();

    component.confirmLegacyFirmwareUpdate();

    expect(otaSpy).toHaveBeenCalledWith(event.files[0]);
    expect(component.pendingFirmwareFile).toBeNull();
    expect(component.firmwareCompatibilityModal?.isVisible).toBeFalse();
  });

  it('should cancel a pending legacy firmware upload', () => {
    const otaSpy = spyOn(systemService, 'performOTAUpdate');

    component.otaUpdate(createUploadEvent('esp-miner.bin'), '302');
    component.cancelLegacyFirmwareUpdate();

    expect(otaSpy).not.toHaveBeenCalled();
    expect(component.pendingFirmwareFile).toBeNull();
    expect(component.firmwareCompatibilityModal?.isVisible).toBeFalse();
  });
});

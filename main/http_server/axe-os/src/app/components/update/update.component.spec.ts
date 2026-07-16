import { ElementRef } from '@angular/core';
import { HttpEventType, provideHttpClient } from '@angular/common/http';
import { ComponentFixture, TestBed } from '@angular/core/testing';
import { provideToastr, ToastrService } from 'ngx-toastr';
import { of } from 'rxjs';

import { CheckboxComponent } from '../checkbox/checkbox.component';
import { ModalComponent } from '../modal/modal.component';
import { SystemApiService } from 'src/app/services/system.service';
import { UpdateComponent } from './update.component';

describe('UpdateComponent', () => {
  let component: UpdateComponent;
  let fixture: ComponentFixture<UpdateComponent>;
  let systemService: SystemApiService;
  let toastrService: ToastrService;
  let firmwareInput: HTMLInputElement;
  let websiteInput: HTMLInputElement;

  const createFile = (filename: string): File => new File(['firmware'], filename);
  const createFileSelectionEvent = (file: File): Event => ({
    target: { files: [file] },
  } as unknown as Event);

  beforeEach(() => {
    TestBed.configureTestingModule({
      declarations: [UpdateComponent, ModalComponent],
      imports: [CheckboxComponent],
      providers: [provideHttpClient(), provideToastr()]
    });
    fixture = TestBed.createComponent(UpdateComponent);
    component = fixture.componentInstance;
    systemService = TestBed.inject(SystemApiService);
    toastrService = TestBed.inject(ToastrService);
    fixture.detectChanges();

    firmwareInput = document.createElement('input');
    websiteInput = document.createElement('input');
    component.firmwareUpload = new ElementRef(firmwareInput);
    component.websiteUpload = new ElementRef(websiteInput);
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

  it('should route native file selections to the correct update handler', () => {
    const firmwareFile = createFile('esp-miner-mcn16r2.bin');
    const websiteFile = createFile('www.bin');
    const firmwareSpy = spyOn(component, 'otaUpdate');
    const websiteSpy = spyOn(component, 'otaWWWUpdate');

    component.onFileSelected(createFileSelectionEvent(firmwareFile), 'firmwareUpload', '312');
    component.onFileSelected(createFileSelectionEvent(websiteFile), 'websiteUpload');

    expect(firmwareSpy).toHaveBeenCalledOnceWith(firmwareFile, '312');
    expect(websiteSpy).toHaveBeenCalledOnceWith(websiteFile);
  });

  it('should upload matching module firmware immediately and clear the input', () => {
    const otaSpy = spyOn(systemService, 'performOTAUpdate').and.returnValue(of({ type: HttpEventType.Response, ok: true } as any));
    const mcn16r2File = createFile('esp-miner-mcn16r2.bin');
    const mcn16r8File = createFile('esp-miner-mcn16r8.bin');

    firmwareInput.value = 'selected';
    component.otaUpdate(mcn16r2File, '312');
    expect(firmwareInput.value).toBe('');

    firmwareInput.value = 'selected';
    component.otaUpdate(mcn16r8File, '302');

    expect(otaSpy).toHaveBeenCalledWith(mcn16r2File);
    expect(otaSpy).toHaveBeenCalledWith(mcn16r8File);
    expect(firmwareInput.value).toBe('');
    expect(component.pendingFirmwareFile).toBeNull();
  });

  it('should reject mismatched and non-firmware files', () => {
    const otaSpy = spyOn(systemService, 'performOTAUpdate');
    const errorSpy = spyOn(toastrService, 'error');

    component.otaUpdate(createFile('esp-miner-mcn16r8.bin'), '312');
    component.otaUpdate(createFile('esp-miner-mcn16r2.bin'), '302');
    component.otaUpdate(createFile('esp-miner-factory-lv07.bin'), '312');
    component.otaUpdate(createFile('www.bin'), '312');

    expect(otaSpy).not.toHaveBeenCalled();
    expect(errorSpy).toHaveBeenCalledTimes(4);
  });

  it('should require confirmation before uploading legacy firmware', () => {
    const otaSpy = spyOn(systemService, 'performOTAUpdate').and.returnValue(of({ type: HttpEventType.Response, ok: true } as any));
    const file = createFile('esp-miner.bin');

    component.otaUpdate(file, '312');

    expect(otaSpy).not.toHaveBeenCalled();
    expect(component.pendingFirmwareFile).toBe(file);
    expect(component.firmwareCompatibilityModal?.isVisible).toBeTrue();

    component.confirmLegacyFirmwareUpdate();

    expect(otaSpy).toHaveBeenCalledWith(file);
    expect(component.pendingFirmwareFile).toBeNull();
    expect(component.firmwareCompatibilityModal?.isVisible).toBeFalse();
  });

  it('should cancel a pending legacy firmware upload', () => {
    const otaSpy = spyOn(systemService, 'performOTAUpdate');

    component.otaUpdate(createFile('esp-miner.bin'), '302');
    component.cancelLegacyFirmwareUpdate();

    expect(otaSpy).not.toHaveBeenCalled();
    expect(component.pendingFirmwareFile).toBeNull();
    expect(component.firmwareCompatibilityModal?.isVisible).toBeFalse();
  });
});

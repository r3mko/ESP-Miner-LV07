import { ElementRef } from '@angular/core';
import { HttpErrorResponse, HttpEventType, provideHttpClient } from '@angular/common/http';
import { ComponentFixture, TestBed } from '@angular/core/testing';
import { FormsModule } from '@angular/forms';
import { ProgressbarComponent } from '../progressbar/progressbar.component';
import { provideToastr, ToastrService } from 'ngx-toastr';
import { of, Subject, throwError } from 'rxjs';

import { CheckboxComponent } from '../checkbox/checkbox.component';
import { ModalComponent } from '../modal/modal.component';
import { SystemInfo } from 'src/app/generated/models';
import { LocalStorageService } from 'src/app/local-storage.service';
import { LiveDataService } from 'src/app/services/live-data.service';
import { SystemApiService } from 'src/app/services/system.service';
import { getHttpErrorMessage } from 'src/app/utils/error-handler';
import { UpdateComponent } from './update.component';
import { GithubRelease, GithubUpdateService } from 'src/app/services/github-update.service';

describe('UpdateComponent', () => {
  let component: UpdateComponent;
  let fixture: ComponentFixture<UpdateComponent>;
  let systemService: SystemApiService;
  let toastrService: ToastrService;
  let firmwareInput: HTMLInputElement;
  let websiteInput: HTMLInputElement;
  let infoSubject: Subject<SystemInfo>;
  let connectedSubject: Subject<boolean>;

  const createFile = (filename: string): File => new File(['firmware'], filename);
  const createFileSelectionEvent = (file: File): Event => ({
    target: { files: [file] },
  } as unknown as Event);

  beforeEach(() => {
    infoSubject = new Subject<SystemInfo>();
    connectedSubject = new Subject<boolean>();

    TestBed.configureTestingModule({
      declarations: [UpdateComponent, ModalComponent],
      imports: [CheckboxComponent, ProgressbarComponent, FormsModule],
      providers: [
        provideHttpClient(),
        provideToastr(),
        // Keep live polling from triggering a real page reload after a mocked upload.
        {
          provide: LiveDataService,
          useValue: {
            info$: infoSubject.asObservable(),
            connected$: connectedSubject.asObservable(),
          },
        },
      ]
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

  it('should stop listening for reload triggers when destroyed', () => {
    expect(infoSubject.observed).toBeTrue();
    expect(connectedSubject.observed).toBeTrue();

    fixture.destroy();

    expect(infoSubject.observed).toBeFalse();
    expect(connectedSubject.observed).toBeFalse();
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

  describe('getHttpErrorMessage', () => {
    it('should format HttpErrorResponse with status 0 as network error', () => {
      const err = new HttpErrorResponse({ status: 0, statusText: 'Unknown Error' });
      const msg = getHttpErrorMessage(err);
      expect(msg).toBe('Network error or connection lost. The device may have restarted or disconnected.');
    });

    it('should format HttpErrorResponse with string error body', () => {
      const err = new HttpErrorResponse({ status: 500, error: 'Write Error' });
      const msg = getHttpErrorMessage(err);
      expect(msg).toBe('Write Error');
    });

    it('should format HttpErrorResponse with JSON error body containing message', () => {
      const err = new HttpErrorResponse({ status: 500, error: { message: 'Out of flash memory' } });
      const msg = getHttpErrorMessage(err);
      expect(msg).toBe('Out of flash memory');
    });

    it('should format HttpErrorResponse with ProgressEvent error body', () => {
      const progressEvent = new ProgressEvent('error');
      const err = new HttpErrorResponse({ status: 500, error: progressEvent, statusText: 'Server Error' });
      const msg = getHttpErrorMessage(err);
      expect(msg).toBe('Upload failed: network error or connection closed.');
    });

    it('should format generic Error object message', () => {
      const err = new Error('Disk full');
      const msg = getHttpErrorMessage(err);
      expect(msg).toBe('Disk full');
    });

    it('should return string directly', () => {
      const msg = getHttpErrorMessage('Custom direct string error');
      expect(msg).toBe('Custom direct string error');
    });

    it('should return fallback message for null/undefined/other types', () => {
      expect(getHttpErrorMessage(null)).toBe('An unknown error occurred.');
      expect(getHttpErrorMessage(undefined)).toBe('An unknown error occurred.');
      expect(getHttpErrorMessage(123)).toBe('An unknown error occurred.');
    });

    it('should append device URI if provided', () => {
      const err = new HttpErrorResponse({ status: 500, error: 'Write Error' });
      const msg = getHttpErrorMessage(err, '192.168.1.10');
      expect(msg).toBe('Write Error (Device: 192.168.1.10)');
    });
  });

  describe('verifyFirmware', () => {
    const sha = 'ab'.repeat(32);
    let githubService: GithubUpdateService;

    const release = (digest?: string | null, filename = 'esp-miner-mcn16r8.bin'): GithubRelease => ({
      id: 1,
      tag_name: 'v2.13.0',
      name: 'v2.13.0',
      html_url: 'https://github.com/r3mko/esp-miner-lv07/releases/tag/v2.13.0',
      prerelease: false,
      assets: [{ name: filename, browser_download_url: '', digest }]
    });

    beforeEach(() => {
      spyOn(TestBed.inject(SystemApiService), 'getFirmwareChecksum').and.returnValue(
        of({ partition: 'ota_0', version: 'v2.13.0', size: 1024, sha256: sha })
      );
      githubService = TestBed.inject(GithubUpdateService);
    });

    it('should report a match when the release digest equals the device checksum', () => {
      const spy = spyOn(githubService, 'getReleaseByTag').and.returnValue(of(release(`sha256:${sha.toUpperCase()}`)));
      component.verifyFirmware('302');
      expect(spy).toHaveBeenCalledWith('v2.13.0');
      expect(component.verifyStatus).toBe('match');
      expect(component.releaseChecksum).toBe(sha);
      expect(component.verifyReleaseUrl).toBe(release().html_url);
    });

    for (const boardVersion of ['312', '302']) {
      it(`should verify only the compatible module firmware for board ${boardVersion}`, () => {
        const expectedFilename = boardVersion === '312' ? 'esp-miner-mcn16r2.bin' : 'esp-miner-mcn16r8.bin';
        const otherFilename = boardVersion === '312' ? 'esp-miner-mcn16r8.bin' : 'esp-miner-mcn16r2.bin';
        const mixedRelease = release(`sha256:${'cd'.repeat(32)}`, otherFilename);
        mixedRelease.assets.push(
          { name: 'esp-miner.bin', browser_download_url: '', digest: `sha256:${'ef'.repeat(32)}` },
          { name: expectedFilename, browser_download_url: '', digest: `sha256:${sha}` },
        );
        spyOn(githubService, 'getReleaseByTag').and.returnValue(of(mixedRelease));

        component.verifyFirmware(boardVersion);

        expect(component.verifyStatus).toBe('match');
        expect(component.releaseChecksum).toBe(sha);
      });
    }

    it('should report a mismatch when the digests differ', () => {
      spyOn(githubService, 'getReleaseByTag').and.returnValue(of(release(`sha256:${'cd'.repeat(32)}`)));
      component.verifyFirmware('302');
      expect(component.verifyStatus).toBe('mismatch');
    });

    it('should report no-digest when the release asset has no digest', () => {
      spyOn(githubService, 'getReleaseByTag').and.returnValue(of(release(null)));
      component.verifyFirmware('302');
      expect(component.verifyStatus).toBe('no-digest');
    });

    it('should not verify against legacy or incompatible firmware when the compatible asset is missing', () => {
      const incompatibleRelease = release(`sha256:${sha}`);
      incompatibleRelease.assets.push({ name: 'esp-miner.bin', browser_download_url: '', digest: `sha256:${sha}` });
      spyOn(githubService, 'getReleaseByTag').and.returnValue(of(incompatibleRelease));

      component.verifyFirmware('312');

      expect(component.verifyStatus).toBe('no-digest');
      expect(component.releaseChecksum).toBeNull();
    });

    it('should report no-release when the tag does not exist', () => {
      spyOn(githubService, 'getReleaseByTag').and.returnValue(throwError(() => new HttpErrorResponse({ status: 404 })));
      component.verifyFirmware('302');
      expect(component.verifyStatus).toBe('no-release');
    });

    it('should report an error for other failures', () => {
      spyOn(githubService, 'getReleaseByTag').and.returnValue(throwError(() => new HttpErrorResponse({ status: 500 })));
      component.verifyFirmware('302');
      expect(component.verifyStatus).toBe('error');
    });
  });

  describe('GitHub privacy warning', () => {
    let localStorageService: LocalStorageService;

    beforeEach(() => {
      localStorageService = TestBed.inject(LocalStorageService);
      spyOn(localStorageService, 'getBool').and.returnValue(false);
      spyOn(localStorageService, 'setBool');
    });

    it('should retain the selected board while waiting for verification consent', () => {
      const verifySpy = spyOn(component, 'verifyFirmware');

      component.handleFirmwareVerify('312');

      expect(verifySpy).not.toHaveBeenCalled();
      expect(component.privacyModal?.isVisible).toBeTrue();

      component.continueReleaseCheck(false);

      expect(verifySpy).toHaveBeenCalledOnceWith('312');
      expect(component.checkLatestRelease).toBeFalse();
      expect(component.privacyModal?.isVisible).toBeFalse();
      expect(localStorageService.setBool).not.toHaveBeenCalled();
    });

    it('should still check releases and remember consent when requested', () => {
      component.handleReleaseCheck();

      expect(component.checkLatestRelease).toBeFalse();
      expect(component.privacyModal?.isVisible).toBeTrue();

      component.continueReleaseCheck(true);

      expect(component.checkLatestRelease).toBeTrue();
      expect(localStorageService.setBool).toHaveBeenCalledOnceWith('IGNORE_RELEASE_CHECK_WARNING', true);
    });

    it('should verify immediately when the privacy warning was previously dismissed', () => {
      (localStorageService.getBool as jasmine.Spy).and.returnValue(true);
      const verifySpy = spyOn(component, 'verifyFirmware');

      component.handleFirmwareVerify('302');

      expect(verifySpy).toHaveBeenCalledOnceWith('302');
      expect(component.privacyModal?.isVisible).toBeFalse();
    });
  });
});

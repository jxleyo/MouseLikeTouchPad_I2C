#include "MouseLikeTouchPad_I2C.h"
#include<math.h>
extern "C" int _fltused = 0;
#define debug_on 1

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry )
#pragma alloc_text(PAGE, OnDeviceAdd)
#pragma alloc_text(PAGE, OnDriverUnload)

#pragma alloc_text(PAGE, AcpiInitialize)
#pragma alloc_text(PAGE, AcpiGetDeviceInformation)
#pragma alloc_text(PAGE, AcpiGetDeviceMethod)
#pragma alloc_text(PAGE, AcpiDsmSupported)
#pragma alloc_text(PAGE, AcpiPrepareInputParametersForDsm)

#pragma alloc_text(PAGE, OnD0Exit)

#endif


VOID RegDebug(WCHAR* strValueName, PVOID dataValue, ULONG datasizeValue)//RegDebug(L"Run debug here",pBuffer,pBufferSize);//RegDebug(L"Run debug here",NULL,0x12345678);
{
    if (!debug_on) {//调试开关
        return;
    }

    //初始化注册表项
    UNICODE_STRING stringKey;
    RtlInitUnicodeString(&stringKey, L"\\Registry\\Machine\\Software\\RegDebug");

    //初始化OBJECT_ATTRIBUTES结构
    OBJECT_ATTRIBUTES  ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes, &stringKey, OBJ_CASE_INSENSITIVE, NULL, NULL);//OBJ_CASE_INSENSITIVE对大小写敏感

    //创建注册表项
    HANDLE hKey;
    ULONG Des;
    NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, &Des);
    if (NT_SUCCESS(status))
    {
        if (Des == REG_CREATED_NEW_KEY)
        {
            KdPrint(("新建注册表项！\n"));
        }
        else
        {
            KdPrint(("要创建的注册表项已经存在！\n"));
        }
    }
    else {
        return;
    }

    //初始化valueName
    UNICODE_STRING valueName;
    RtlInitUnicodeString(&valueName, strValueName);

    if (dataValue == NULL) {
        //设置REG_DWORD键值
        status = ZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &datasizeValue, 4);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("设置REG_DWORD键值失败！\n"));
        }
    }
    else {
        //设置REG_BINARY键值
        status = ZwSetValueKey(hKey, &valueName, 0, REG_BINARY, dataValue, datasizeValue);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("设置REG_BINARY键值失败！\n"));
        }
    }
    ZwFlushKey(hKey);
    //关闭注册表句柄
    ZwClose(hKey);
}


EXTERN_C
NTSTATUS
AcpiInitialize(
    _In_ PDEVICE_CONTEXT    FxDeviceContext
)
{
    NTSTATUS status;

    PAGED_CODE();

    status = ::AcpiGetDeviceMethod(FxDeviceContext);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"EventWrite_HIDI2C_ENUMERATION_ACPI_FAILURE", NULL, status);
    }

    RegDebug(L"AcpiInitialize ok", NULL, status);
    return status;
}

EXTERN_C
NTSTATUS
AcpiGetDeviceInformation(
    _In_ PDEVICE_CONTEXT    FxDeviceContext
)
{
    NTSTATUS status;

    PAGED_CODE();

    WDFIOTARGET acpiIoTarget;
    acpiIoTarget = WdfDeviceGetIoTarget(FxDeviceContext->FxDevice);

    PACPI_DEVICE_INFORMATION_OUTPUT_BUFFER acpiInfo = NULL;

    ULONG acpiInfoSize;
    acpiInfoSize = sizeof(ACPI_DEVICE_INFORMATION_OUTPUT_BUFFER) +
        EXPECTED_IOCTL_ACPI_GET_DEVICE_INFORMATION_STRING_LENGTH;

    acpiInfo = (PACPI_DEVICE_INFORMATION_OUTPUT_BUFFER)
        HIDI2C_ALLOCATE_POOL(NonPagedPoolNx, acpiInfoSize);

    if (acpiInfo == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"Failed allocating memory for ACPI output buffer", NULL, status);
        goto exit;
    }

    WDF_MEMORY_DESCRIPTOR outputDescriptor;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outputDescriptor,
        (PVOID)acpiInfo,
        acpiInfoSize);

    status = WdfIoTargetSendIoctlSynchronously(
        acpiIoTarget,
        NULL,
        IOCTL_ACPI_GET_DEVICE_INFORMATION,
        NULL,
        &outputDescriptor,
        NULL,
        NULL);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"Failed sending IOCTL_ACPI_GET_DEVICE_INFORMATION", NULL, status);
        goto exit;
    }

    if (acpiInfo->Signature != IOCTL_ACPI_GET_DEVICE_INFORMATION_SIGNATURE)
    {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"Incorrect signature for ACPI_GET_DEVICE_INFORMATION", NULL, status);
        goto exit;
    }

exit:

    if (acpiInfo != NULL)
        HIDI2C_FREE_POOL(acpiInfo);

    RegDebug(L"_AcpiGetDeviceInformation end", NULL, status);
    return status;
}


EXTERN_C
NTSTATUS
AcpiGetDeviceMethod(
    _In_ PDEVICE_CONTEXT    FxDeviceContext
)
{
    NTSTATUS status;

    PAGED_CODE();

    BOOLEAN                         isSupported = FALSE;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX acpiInput = NULL;
    ULONG                           acpiInputSize;
    ACPI_EVAL_OUTPUT_BUFFER         acpiOutput;

    status = AcpiDsmSupported(
        FxDeviceContext,
        ACPI_DSM_HIDI2C_FUNCTION,
        &isSupported);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"AcpiDsmSupported err", NULL, status);
        goto exit;
    }

    if (isSupported == FALSE)
    {
        status = STATUS_NOT_SUPPORTED;
        RegDebug(L"Check for DSM support returned err", NULL, status);
        goto exit;
    }

    acpiInputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
        (sizeof(ACPI_METHOD_ARGUMENT) *
            (ACPI_DSM_INPUT_ARGUMENTS_COUNT - 1)) +
        sizeof(GUID);//检查是否=0x40
    RegDebug(L"AcpiGetDeviceMethod acpiInputSize", NULL, acpiInputSize);////检查是否=0x40

    acpiInput = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)HIDI2C_ALLOCATE_POOL(PagedPool, acpiInputSize);

    if (acpiInput == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"Failed allocating memory for ACPI Input buffer", NULL, status);
        goto exit;
    }

    ::AcpiPrepareInputParametersForDsm(
        acpiInput,
        acpiInputSize,
        ACPI_DSM_HIDI2C_FUNCTION);

    RtlZeroMemory(&acpiOutput, sizeof(ACPI_EVAL_OUTPUT_BUFFER));


    WDF_MEMORY_DESCRIPTOR  inputDescriptor;
    WDF_MEMORY_DESCRIPTOR  outputDescriptor;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inputDescriptor,
        (PVOID)acpiInput,
        acpiInputSize);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outputDescriptor,
        (PVOID)&acpiOutput,
        sizeof(ACPI_EVAL_OUTPUT_BUFFER));

    WDFIOTARGET acpiIoTarget;
    acpiIoTarget = WdfDeviceGetIoTarget(FxDeviceContext->FxDevice);

    NT_ASSERTMSG("ACPI IO target is NULL", acpiIoTarget != NULL);

    status = WdfIoTargetSendIoctlSynchronously(
        acpiIoTarget,
        NULL,
        IOCTL_ACPI_EVAL_METHOD,
        &inputDescriptor,
        &outputDescriptor,
        NULL,
        NULL);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"Failed executing DSM method IOCTL", NULL, status);
        goto exit;
    }

    if ((acpiOutput.Argument[0].Type != ACPI_METHOD_ARGUMENT_INTEGER) ||
        (acpiOutput.Argument[0].DataLength == 0))
    {
        status = STATUS_UNSUCCESSFUL;
        RegDebug(L"Incorrect parameters returned for DSM method", NULL, status);
        goto exit;
    }

    FxDeviceContext->AcpiSettings.HidDescriptorAddress = (USHORT)acpiOutput.Argument[0].Data[0];

    //RegDebug(L"HID Descriptor offset retrieved from ACPI", NULL, FxDeviceContext->AcpiSettings.HidDescriptorAddress);


exit:

    if (acpiInput != NULL)
        HIDI2C_FREE_POOL(acpiInput);

    //RegDebug(L"AcpiGetDeviceMethod end", NULL, status);
    return status;
}


EXTERN_C
NTSTATUS
AcpiDsmSupported(
    _In_ PDEVICE_CONTEXT    FxDeviceContext,
    _In_ ULONG              FunctionIndex,
    _Out_ PBOOLEAN          Supported
)
{
    NTSTATUS status;

    ACPI_EVAL_OUTPUT_BUFFER         acpiOutput;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX acpiInput;
    ULONG                           acpiInputSize;

    PAGED_CODE();

    BOOLEAN support = FALSE;

    acpiInputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
        (sizeof(ACPI_METHOD_ARGUMENT) * (ACPI_DSM_INPUT_ARGUMENTS_COUNT - 1)) +
        sizeof(GUID);

    acpiInput = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)HIDI2C_ALLOCATE_POOL(PagedPool, acpiInputSize);

    if (acpiInput == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"Failed allocating memory for ACPI input buffer", NULL, status);
        goto exit;
    }

    ::AcpiPrepareInputParametersForDsm(
        acpiInput,
        acpiInputSize,
        ACPI_DSM_QUERY_FUNCTION);

    RtlZeroMemory(&acpiOutput, sizeof(ACPI_EVAL_OUTPUT_BUFFER));

    WDF_MEMORY_DESCRIPTOR  inputDescriptor;
    WDF_MEMORY_DESCRIPTOR  outputDescriptor;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inputDescriptor,
        (PVOID)acpiInput,
        acpiInputSize);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outputDescriptor,
        (PVOID)&acpiOutput,
        sizeof(ACPI_EVAL_OUTPUT_BUFFER));

    WDFIOTARGET acpiIoTarget;
    acpiIoTarget = WdfDeviceGetIoTarget(FxDeviceContext->FxDevice);

    NT_ASSERTMSG("ACPI IO target is NULL", acpiIoTarget != NULL);

    status = WdfIoTargetSendIoctlSynchronously(
        acpiIoTarget,
        NULL,
        IOCTL_ACPI_EVAL_METHOD,
        &inputDescriptor,
        &outputDescriptor,
        NULL,
        NULL);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"Failed sending DSM function query IOCTL", NULL, status);
        goto exit;
    }

    if ((acpiOutput.Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER) ||
        (acpiOutput.Argument[0].DataLength == 0))
    {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"Incorrect return parameters for DSM function query IOCTL", NULL, status);
        goto exit;
    }

    status = STATUS_SUCCESS;
    if ((acpiOutput.Argument[0].Data[0] & (1 << FunctionIndex)) != 0)
    {
        support = TRUE;
        RegDebug(L"Found supported DSM function", NULL, FunctionIndex);
    }

exit:

    if (acpiInput != NULL)
    {
        HIDI2C_FREE_POOL(acpiInput);
    }

    //RegDebug(L"_AcpiDsmSupported end", NULL, status);
    *Supported = support;
    return status;
}


VOID
AcpiPrepareInputParametersForDsm(
    _Inout_ PACPI_EVAL_INPUT_BUFFER_COMPLEX ParametersBuffer,
    _In_ ULONG ParametersBufferSize,
    _In_ ULONG FunctionIndex
)
{
    PACPI_METHOD_ARGUMENT pArgument;

    PAGED_CODE();

    NT_ASSERTMSG("ACPI input buffer is NULL", ParametersBuffer != NULL);

    RtlZeroMemory(ParametersBuffer, ParametersBufferSize);

    ParametersBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    ParametersBuffer->MethodNameAsUlong = (ULONG)'MSD_';
    ParametersBuffer->Size = ParametersBufferSize;
    ParametersBuffer->ArgumentCount = ACPI_DSM_INPUT_ARGUMENTS_COUNT;

    pArgument = &ParametersBuffer->Argument[0];
    ACPI_METHOD_SET_ARGUMENT_BUFFER(pArgument, &ACPI_DSM_GUID_HIDI2C, sizeof(GUID));

    pArgument = ACPI_METHOD_NEXT_ARGUMENT(pArgument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(pArgument, ACPI_DSM_REVISION);

    pArgument = ACPI_METHOD_NEXT_ARGUMENT(pArgument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(pArgument, FunctionIndex);

    pArgument = ACPI_METHOD_NEXT_ARGUMENT(pArgument);
    pArgument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
    pArgument->DataLength = sizeof(ULONG);
    pArgument->Argument = 0;

    //RegDebug(L"AcpiPrepareInputParametersForDsm end", NULL, runtimes_hid++);
    return;
}


NTSTATUS
DriverEntry(_DRIVER_OBJECT* DriverObject, PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG   DriverConfig;
	WDF_DRIVER_CONFIG_INIT(&DriverConfig, OnDeviceAdd);
    
	DriverConfig.EvtDriverUnload = OnDriverUnload;
    DriverConfig.DriverPoolTag = HIDI2C_POOL_TAG;
	NTSTATUS status = WdfDriverCreate(DriverObject,
		RegistryPath,
		NULL,
		&DriverConfig,
		WDF_NO_HANDLE);

	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfDriverCreate failed", NULL, status);
		return status;
	}

	return status;

}


void OnDriverUnload(_In_ WDFDRIVER Driver)
{

	PDRIVER_OBJECT pDriverObject= WdfDriverWdmGetDriverObject(Driver);

    PAGED_CODE();
	UNREFERENCED_PARAMETER(pDriverObject);

	RegDebug(L"OnDriverUnload", NULL, 0);
}


NTSTATUS OnDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, FALSE);

	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	pnpPowerCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = OnD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = OnD0Exit;
	pnpPowerCallbacks.EvtDeviceD0EntryPostInterruptsEnabled= OnPostInterruptsEnabled;
	pnpPowerCallbacks.EvtDeviceSelfManagedIoSuspend =OnSelfManagedIoSuspend;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES DeviceAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, DEVICE_CONTEXT);

	
	WDFDEVICE Device;
	status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &Device);
	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfDeviceCreate failed", NULL, status);
		return status;
	}

	PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);
	pDevContext->FxDevice = Device;


    WDF_TIMER_CONFIG   timerConfig;
    WDF_TIMER_CONFIG_INIT(&timerConfig, HidEvtResetTimerFired);//默认 串行执行timerFunc
    timerConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES   timerAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    timerAttributes.ParentObject = Device;

    status = WdfTimerCreate(&timerConfig, &timerAttributes, &pDevContext->timerHandle);// WDFTIMER  
    if (!NT_SUCCESS(status)) {
        RegDebug(L"WdfTimerCreate failed", NULL, status);
        return status;
    }

    //WDF_OBJECT_ATTRIBUTES spinLockAttributes;
    //WDF_OBJECT_ATTRIBUTES_INIT(&spinLockAttributes);

    //spinLockAttributes.ParentObject = Device;

    //status = WdfSpinLockCreate(&spinLockAttributes, &pDevContext->DeviceResetNotificationSpinLock);

    //if (!NT_SUCCESS(status))
    //{
    //    //printf("WdfSpinLockCreate failed");
    //    return status;
    //}
    //pDevContext->DeviceResetNotificationRequest = NULL;

    WDF_DEVICE_STATE deviceState;
    WDF_DEVICE_STATE_INIT(&deviceState);
    deviceState.NotDisableable = WdfFalse;//增加禁用驱动功能
    WdfDeviceSetDeviceState(Device, &deviceState);
   

	WDF_IO_QUEUE_CONFIG  queueConfig;

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
	queueConfig.EvtIoInternalDeviceControl = OnInternalDeviceIoControl;
	queueConfig.EvtIoStop = OnIoStop;

	status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->IoctlQueue);
	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfIoQueueCreate IoctlQueue failed", NULL, status);
		return status;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;
	status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->ReportQueue);
	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfIoQueueCreate ReportQueue failed", NULL, status);
		return status;
	}

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;
    status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->CompletionQueue);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"WdfIoQueueCreate CompletionQueue failed", NULL, status);
        return status;
    }

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;
	status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->IdleQueue);
	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfIoQueueCreate IdleQueue failed", NULL, status);
		return status;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    queueConfig.PowerManaged = WdfFalse;
	status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->ResetNotificationQueue);
	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfIoQueueCreate ResetNotificationQueue failed", NULL, status);
		return status;
	}

	//RegDebug(L"OnDeviceAdd ok", NULL, 0);
	return STATUS_SUCCESS;

}


void HidEvtResetTimerFired(WDFTIMER timer)
{
    runtimes_HidEvtResetTimerFired++;

    WDFDEVICE Device = (WDFDEVICE)WdfTimerGetParentObject(timer);

    PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);

    DECLARE_CONST_UNICODE_STRING(ValueNameString, L"DoNotWaitForResetResponse");

    WdfIoQueueStart(pDevContext->IoctlQueue);//IoctlQueue//

    ////新增
    //if (!pDevContext->HostInitiatedResetActive) {
    //    WDFTIMER  timerHandle = pDevContext->timerHandle;
    //    if (timerHandle) {
    //        RegDebug(L"HidEvtResetTimerFired WdfTimerStop", NULL, runtimes_HidEvtResetTimerFired);
    //        WdfTimerStop(pDevContext->timerHandle, TRUE);
    //    }
    //}

    WDFKEY hKey = NULL;

    //伪代码很可能少了2个参数??
    NTSTATUS status = WdfDeviceOpenRegistryKey(
        Device,
        PLUGPLAY_REGKEY_DEVICE,//1
        KEY_WRITE,
        WDF_NO_OBJECT_ATTRIBUTES,
        &hKey);

        //WdfDeviceOpenDevicemapKey(
        //	device,
        //	&ValueNameString,
        //	KEY_WRITE,
        //	WDF_NO_OBJECT_ATTRIBUTES,
        //	&hKey);

    if (NT_SUCCESS(status)) {
        WdfRegistryAssignULong(hKey, &ValueNameString, 1);
        RegDebug(L"HidEvtResetTimerFired WdfRegistryAssignULong ok", NULL, runtimes_HidEvtResetTimerFired);
    }

    if (hKey)
        WdfObjectDelete(hKey);

    RegDebug(L"HidEvtResetTimerFired end", NULL, status);
}


VOID OnInternalDeviceIoControl(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t       OutputBufferLength,
    IN size_t       InputBufferLength,
    IN ULONG        IoControlCode
)
{
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE Device = WdfIoQueueGetDevice(Queue);

    PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);

    HID_REPORT_TYPE hidReportType = ReportTypeReserved;
    UNREFERENCED_PARAMETER(hidReportType);

    BOOLEAN requestPendingFlag = FALSE;

    runtimes_ioControl++;
    //if (runtimes_ioControl == 1) {
    //    RegDebug(L"IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST IoControlCode", NULL, IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST);//0xb002b
    //    RegDebug(L"IOCTL_HID_GET_DEVICE_DESCRIPTOR IoControlCode", NULL, IOCTL_HID_GET_DEVICE_DESCRIPTOR);//0xb0003
    //    RegDebug(L"IOCTL_HID_GET_REPORT_DESCRIPTOR IoControlCode", NULL, IOCTL_HID_GET_REPORT_DESCRIPTOR);//0xb0007
    //    RegDebug(L"IOCTL_HID_GET_DEVICE_ATTRIBUTES IoControlCode", NULL, IOCTL_HID_GET_DEVICE_ATTRIBUTES);//0xb0027
    //    RegDebug(L"IOCTL_HID_READ_REPORT IoControlCode", NULL, IOCTL_HID_READ_REPORT);//0xb000b
    //    RegDebug(L"IOCTL_HID_WRITE_REPORT IoControlCode", NULL, IOCTL_HID_WRITE_REPORT);//0xb000f
    //    RegDebug(L"IOCTL_HID_GET_STRING IoControlCode", NULL, IOCTL_HID_GET_STRING);//0xb0013
    //    RegDebug(L"IOCTL_HID_GET_FEATURE IoControlCode", NULL, IOCTL_HID_GET_FEATURE);//0xb0192
    //    RegDebug(L"IOCTL_HID_SET_FEATURE IoControlCode", NULL, IOCTL_HID_SET_FEATURE);//0xb0191
    //    RegDebug(L"IOCTL_HID_GET_INPUT_REPORT IoControlCode", NULL, IOCTL_HID_GET_INPUT_REPORT);//0xb01a2
    //    RegDebug(L"IOCTL_HID_SET_OUTPUT_REPORT IoControlCode", NULL, IOCTL_HID_SET_OUTPUT_REPORT);//0xb0195
    //    RegDebug(L"IOCTL_HID_DEVICERESET_NOTIFICATION IoControlCode", NULL, IOCTL_HID_DEVICERESET_NOTIFICATION);//0xb0233
    //}
    
    switch (IoControlCode)
    {
        
        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        {
            RegDebug(L"IOCTL_HID_GET_DEVICE_DESCRIPTOR", NULL, runtimes_ioControl);

            WDFMEMORY memory;
            status = WdfRequestRetrieveOutputMemory(Request, &memory);
            if (!NT_SUCCESS(status)) {
                RegDebug(L"IOCTL_HID_GET_DEVICE_DESCRIPTOR WdfRequestRetrieveOutputMemory err", NULL, runtimes_ioControl);
                break;
            }

            //USHORT ReportDescriptorLength = pDevContext->HidSettings.ReportDescriptorLength;//设置描述符长度
            //HID_DESCRIPTOR HidDescriptor = {0x09, 0x21, 0x0100, 0x00, 0x01, { 0x22, ReportDescriptorLength }  // HidReportDescriptor
            //};
            //RegDebug(L"IOCTL_HID_GET_DEVICE_DESCRIPTOR HidDescriptor=", &HidDescriptor, HidDescriptor.bLength);

            status = WdfMemoryCopyFromBuffer(memory, 0, (PVOID)&DefaultHidDescriptor, DefaultHidDescriptor.bLength);//DefaultHidDescriptor//HidDescriptor
            if (!NT_SUCCESS(status)) {
                RegDebug(L"IOCTL_HID_GET_DEVICE_DESCRIPTOR WdfMemoryCopyFromBuffer err", NULL, runtimes_ioControl);
                break;
            }
            ////
            WdfRequestSetInformation(Request, DefaultHidDescriptor.bLength);//DefaultHidDescriptor//HidDescriptor

            //status = HidGetDeviceDescriptor(pDevContext, Request);
            break;
        }
        

        case IOCTL_HID_GET_REPORT_DESCRIPTOR:  
        {
            RegDebug(L"IOCTL_HID_GET_REPORT_DESCRIPTOR", NULL, runtimes_ioControl);

            WDFMEMORY memory;
            status = WdfRequestRetrieveOutputMemory(Request, &memory);
            if (!NT_SUCCESS(status)) {
                RegDebug(L"OnInternalDeviceIoControl WdfRequestRetrieveOutputMemory err", NULL, runtimes_ioControl);
                break;
            }
    
            //PVOID outbuf = pDevContext->pReportDesciptorData;
            //LONG outlen = pDevContext->HidSettings.ReportDescriptorLength;//设置描述符长度
            //RegDebug(L"IOCTL_HID_GET_REPORT_DESCRIPTOR HidDescriptor=", pDevContext->pReportDesciptorData, outlen);

            PVOID outbuf = (PVOID)ParallelMode_PtpReportDescriptor;//(PVOID)ParallelMode_PtpReportDescriptor //(PVOID)MouseReportDescriptor//(PVOID)SingleFingerHybridMode_PtpReportDescriptor
            LONG outlen = DefaultHidDescriptor.DescriptorList[0].wReportLength;

            status = WdfMemoryCopyFromBuffer(memory, 0, outbuf, outlen);
            if (!NT_SUCCESS(status)) {
                RegDebug(L"IOCTL_HID_GET_REPORT_DESCRIPTOR WdfMemoryCopyFromBuffer err", NULL, runtimes_ioControl);
                break;
            }
            WdfRequestSetInformation(Request, outlen);

            //status = HidGetReportDescriptor(pDevContext, Request);
            break;
        }

        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        {
            RegDebug(L"IOCTL_HID_GET_DEVICE_ATTRIBUTES", NULL, runtimes_ioControl);
            status = HidGetDeviceAttributes(pDevContext, Request);
            break;
        }
        case IOCTL_HID_READ_REPORT:
        {
            //RegDebug(L"IOCTL_HID_READ_REPORT", NULL, runtimes_ioControl);
            //RegDebug(L"IOCTL_HID_READ_REPORT runtimes_IOCTL_HID_READ_REPORT", NULL, runtimes_IOCTL_HID_READ_REPORT++);
            
            //if (pDevContext->SetFeatureReady == TRUE) {//条件判断代码在设置input mode之前
            //            if (pDevContext->SetInputModeOK) {
            //                status = PtpSetFeature(pDevContext, PTP_FEATURE_SELECTIVE_REPORTING);//设置触摸板SELECTIVE_REPORTING
            //                if (!NT_SUCCESS(status)) {
            //                    RegDebug(L"HidSetFeature PTP_SET_FEATURE_SELECTIVE_REPORTING err", NULL, status);
            //                }
            //                else {
            //                    pDevContext->SetFunSwicthOK = TRUE;
            //                }
            //            }

            //            status = PtpSetFeature(pDevContext, PTP_FEATURE_INPUT_COLLECTION);//设置触摸板input mode
            //            if (!NT_SUCCESS(status)) {
            //                RegDebug(L"HidSetFeature PTP_SET_FEATURE_INPUT_COLLECTION err", NULL, status);
            //            }
            //            else {
            //                pDevContext->SetInputModeOK = TRUE;
            //            }

            //            if (pDevContext->SetFunSwicthOK) {
            //                pDevContext->SetFeatureReady = FALSE;
            //                pDevContext->PtpInputModeOn = TRUE;//
            //            }

            //            RegDebug(L"IOCTL_HID_READ_REPORT SetFeatureReady", NULL, runtimes_ioControl);
            //}

            status = HidReadReport(pDevContext, Request, &requestPendingFlag);
            if (requestPendingFlag) {
                return;
            }
            break;
        }   
       
        case IOCTL_HID_GET_FEATURE:
        {
            RegDebug(L"IOCTL_HID_GET_FEATURE", NULL, runtimes_ioControl);

            //status = HidGetFeature(pDevContext, Request, ReportTypeFeature);

            status = PtpReportFeatures(
                Device,
                Request
            );

            //status = HidGetReport(pDevContext, Request, ReportTypeFeature);
            break;
        }    

        case IOCTL_HID_SET_FEATURE:
        {
            RegDebug(L"IOCTL_HID_SET_FEATURE", NULL, runtimes_ioControl);
            
            status = HidSetFeature(pDevContext, Request, ReportTypeFeature);

            //status = HidSetReport(pDevContext, Request, ReportTypeFeature);

            if (requestPendingFlag) {
                return;
            }
            break;
        }
        
        case IOCTL_HID_GET_STRING:
        {
            RegDebug(L"IOCTL_HID_GET_STRING", NULL, runtimes_ioControl);
            //status = STATUS_NOT_IMPLEMENTED;
            status = HidGetString(pDevContext, Request);//代码会死机
            break;
        }  

        case IOCTL_HID_WRITE_REPORT:
        {
            RegDebug(L"IOCTL_HID_WRITE_REPORT", NULL, runtimes_ioControl);
            status = HidWriteReport(pDevContext, Request);
            break;
        }
     
        case IOCTL_HID_GET_INPUT_REPORT:
            RegDebug(L"IOCTL_HID_GET_INPUT_REPORT", NULL, runtimes_ioControl);
            //status = STATUS_NOT_SUPPORTED;
            hidReportType = ReportTypeInput;//1
            status = HidGetReport(pDevContext, Request, hidReportType);
            break;

        case IOCTL_HID_SET_OUTPUT_REPORT:
            RegDebug(L"IOCTL_HID_SET_OUTPUT_REPORT", NULL, runtimes_ioControl);
            status = HidSetReport(pDevContext, Request, ReportTypeOutput);//2
            break;

            // Hidclass sends this IOCTL to notify miniports that it wants
       // them to go into idle
        case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        {
            RegDebug(L"IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST", NULL, runtimes_ioControl);
            status = HidSendIdleNotification(pDevContext, Request, &requestPendingFlag);
            break;
        }

        // HIDCLASS sends this IOCTL to wait for the device-initiated reset event.
       // We only allow one Device Reset Notification request at a time.
        case IOCTL_HID_DEVICERESET_NOTIFICATION:
        {
            RegDebug(L"IOCTL_HID_DEVICERESET_NOTIFICATION", NULL, runtimes_ioControl);
            //BOOLEAN requestPendingFlag_reset = FALSE;
            //status = HidSendResetNotification(pDevContext, Request, &requestPendingFlag_reset);
            //if (requestPendingFlag_reset) {
            //    return;
            //}

            //WdfIoQueueGetState(pDevContext->ResetNotificationQueue, &IoControlCode, NULL);
            status = WdfRequestForwardToIoQueue(Request, pDevContext->ResetNotificationQueue);
            if (NT_SUCCESS(status)){
                return;
            }
                

            ////
            //WdfSpinLockAcquire(pDevContext->DeviceResetNotificationSpinLock);

            //if (pDevContext->DeviceResetNotificationRequest != NULL)
            //{
            //    status = STATUS_INVALID_DEVICE_REQUEST;
            //    requestPendingFlag = FALSE;
            //    RegDebug(L"IOCTL_HID_DEVICERESET_NOTIFICATION STATUS_INVALID_DEVICE_REQUEST", NULL, runtimes_ioControl);
            //}
            //else
            //{
            //    //
            //    // Try to make it cancelable
            //    //
            //    status = WdfRequestMarkCancelableEx(Request, OnDeviceResetNotificationRequestCancel);

            //    if (NT_SUCCESS(status))
            //    {
            //        //
            //        // Successfully marked it cancelable. Pend the request then.
            //        //
            //        pDevContext->DeviceResetNotificationRequest = Request;
            //        status = STATUS_PENDING;    // just to satisfy the compilier
            //        requestPendingFlag = TRUE;
            //        RegDebug(L"IOCTL_HID_DEVICERESET_NOTIFICATION WdfRequestMarkCancelableEx ok", NULL, runtimes_ioControl);
            //    }
            //    else
            //    {
            //        //
            //        // It's already cancelled. Our EvtRequestCancel won't be invoked.
            //        // We have to complete the request with STATUS_CANCELLED here.
            //        //
            //        NT_ASSERT(status == STATUS_CANCELLED);
            //        status = STATUS_CANCELLED;    // just to satisfy the compilier
            //        requestPendingFlag = FALSE;
            //        RegDebug(L"IOCTL_HID_DEVICERESET_NOTIFICATION  STATUS_CANCELLED", NULL, runtimes_ioControl);

            //    }
            //}

            //WdfSpinLockRelease(pDevContext->DeviceResetNotificationSpinLock);
            break;
        }

        default:
        {
            status = STATUS_NOT_SUPPORTED;
            RegDebug(L"STATUS_NOT_SUPPORTED", NULL, IoControlCode);
            RegDebug(L"STATUS_NOT_SUPPORTED FUNCTION_FROM_CTL_CODE", NULL, FUNCTION_FROM_CTL_CODE(IoControlCode));
        }

    }

    //
    // Complete the request if it was not forwarded
    //
    if (requestPendingFlag == FALSE)
    {
        WdfRequestComplete(Request, status);
    }

    //WdfRequestComplete(Request, status);
    return;
}


void OnIoStop(WDFQUEUE Queue, WDFREQUEST Request, ULONG ActionFlags)
{

    UNREFERENCED_PARAMETER(Queue);

    RegDebug(L"OnIoStop start", NULL, runtimes_ioControl);

    //NTSTATUS status;
    //PDEVICE_CONTEXT pDevContext;
    //PHID_XFER_PACKET pHidPacket;
    WDF_REQUEST_PARAMETERS RequestParameters;

    WDF_REQUEST_PARAMETERS_INIT(&RequestParameters);
    WdfRequestGetParameters(Request, &RequestParameters);

    if (RequestParameters.Type == WdfRequestTypeDeviceControlInternal && RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST && (ActionFlags & 1) != 0)//RequestParameters.Parameters.Others.IoControlCode
    {
        RegDebug(L"OnIoStop WdfRequestStopAcknowledge", NULL, runtimes_ioControl);
        WdfRequestStopAcknowledge(Request, 0);
    }
    RegDebug(L"OnIoStop end", NULL, runtimes_ioControl);
}

//
//VOID
//OnIoStop(
//    _In_  WDFQUEUE      FxQueue,
//    _In_  WDFREQUEST    FxRequest,
//    _In_  ULONG         ActionFlags
//)
///*++
//Routine Description:
//
//    OnIoStop is called by the framework when the device leaves the D0 working state
//    for every I/O request that this driver has not completed, including
//    requests that the driver owns and those that it has forwarded to an
//    I/O target.
//
//Arguments:
//
//    FxQueue - Handle to the framework queue object that is associated with the I/O request.
//
//    FxRequest - Handle to a framework request object.
//
//    ActionFlags - WDF_REQUEST_STOP_ACTION_FLAGS specifying reason that the callback function is being called
//
//Return Value:
//
//    None
//
//--*/
//{
//    WDF_REQUEST_PARAMETERS fxParams;
//    WDF_REQUEST_PARAMETERS_INIT(&fxParams);
//
//    // Get the request parameters
//    WdfRequestGetParameters(FxRequest, &fxParams);
//
//    if (fxParams.Type == WdfRequestTypeDeviceControlInternal)
//    {
//        switch (fxParams.Parameters.DeviceIoControl.IoControlCode)
//        {
//            //
//            // When we invoke the HID idle notification callback inside the workitem, the idle IRP is
//            // yet to be enqueued into the Idle Queued. Since hidclass will queue a Dx IRP from the callback
//            // and our default queue is power managed, the Dx IRP progression is blocked because of the in-flight
//            // idle IRP. In order to avoid blocking the Dx IRP, we acknowledge the request. Since 
//            // we anyway will queue the IRP into the Idle Queue immediately after, this is safe. 
//            //
//            case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
//                //
//                // The framework is stopping the I/O queue because the device is leaving its working (D0) state.
//                //
//                if (ActionFlags & WdfRequestStopActionSuspend)
//                {
//                    // Acknowledge the request
//                    //
//                    WdfRequestStopAcknowledge(FxRequest, FALSE);
//                }
//                break;
//
//                //
//                // Device Reset Notification is normally pended by the HIDI2C driver.
//                // We need to complete it only when the device is being removed. For
//                // device power state changes, we will keep it pended, since HID clients 
//                // are not interested in these changes.
//                //
//            case IOCTL_HID_DEVICERESET_NOTIFICATION:
//                if (ActionFlags & WdfRequestStopActionPurge)
//                {
//                    //
//                    // The framework is stopping it because the device is being removed.
//                    // So we complete it, if it's not cancelled yet.
//                    //
//                    PDEVICE_CONTEXT pDeviceContext = GetDeviceContext(WdfIoQueueGetDevice(FxQueue));
//                    BOOLEAN         completeRequest = FALSE;
//                    NTSTATUS        status;
//
//                    WdfSpinLockAcquire(pDeviceContext->DeviceResetNotificationSpinLock);
//
//                    status = WdfRequestUnmarkCancelable(FxRequest);
//                    if (NT_SUCCESS(status))
//                    {
//                        //
//                        // EvtRequestCancel won't be called. We complete it here.
//                        //
//                        completeRequest = TRUE;
//
//                        NT_ASSERT(pDeviceContext->DeviceResetNotificationRequest == FxRequest);
//                        if (pDeviceContext->DeviceResetNotificationRequest == FxRequest)
//                        {
//                            pDeviceContext->DeviceResetNotificationRequest = NULL;
//                        }
//                    }
//                    else
//                    {
//                        NT_ASSERT(status == STATUS_CANCELLED);
//                        // EvtRequestCancel will be called. Leave it as it is.
//                    }
//
//                    WdfSpinLockRelease(pDeviceContext->DeviceResetNotificationSpinLock);
//
//                    if (completeRequest)
//                    {
//                        //
//                        // Complete the pending Device Reset Notification with STATUS_CANCELLED
//                        //
//                        status = STATUS_CANCELLED;
//                        WdfRequestComplete(FxRequest, status);
//                    }
//                }
//                else
//                {
//                    //
//                    // The framework is stopping it because the device is leaving D0.
//                    // Keep it pending.
//                    //
//                    NT_ASSERT(ActionFlags & WdfRequestStopActionSuspend);
//                    WdfRequestStopAcknowledge(FxRequest, FALSE);
//                }
//                break;
//
//            default:
//                //
//                // Leave other requests as they are. 
//                //
//                NOTHING;
//        }
//    }
//}


NTSTATUS OnPostInterruptsEnabled(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
    runtimes_OnPostInterruptsEnabled++;
    RegDebug(L"OnPostInterruptsEnabled runtimes_OnPostInterruptsEnabled ", NULL, runtimes_OnPostInterruptsEnabled);

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);
    NT_ASSERT(pDevContext != NULL);

    if (PreviousState == WdfPowerDeviceD3Final) {
        RegDebug(L"OnPostInterruptsEnabled HidReset", NULL, runtimes_OnPostInterruptsEnabled);

        ////新增代码，安装驱动后首次运行重新加电就无需重启计算机，FirstD0Entry需要在OnPrepareHardware里初始化而不能在OnD0Entry里，每次加电会运行OnD0Entry，
        //if (pDevContext->FirstD0Entry) {

        //    status = HidPower(pDevContext, WdfPowerDeviceD0);
        //    if (!NT_SUCCESS(status))
        //    {
        //        RegDebug(L"OnSelfManagedIoSuspend _HidPower WdfPowerDeviceD0 failed", NULL, status);
        //    }
        //    else {
        //        pDevContext->FirstD0Entry = FALSE;
        //    }
        //}

        status = HidReset(pDevContext);  
    }

    

    UNREFERENCED_PARAMETER(pDevContext);
    RegDebug(L"OnPostInterruptsEnabled end runtimes_hid", NULL, runtimes_hid++);
    return status;
}


NTSTATUS OnSelfManagedIoSuspend(WDFDEVICE Device)
{
    runtimes_OnSelfManagedIoSuspend++;

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);
    UNREFERENCED_PARAMETER(pDevContext);


        WDFTIMER  timerHandle = pDevContext->timerHandle;
        if (timerHandle) {
            RegDebug(L"OnSelfManagedIoSuspend WdfTimerStop", NULL, runtimes_OnSelfManagedIoSuspend);
            WdfTimerStop(pDevContext->timerHandle, TRUE);
        }


    RegDebug(L"OnSelfManagedIoSuspend end runtimes_hid", NULL, runtimes_hid++);
    return status;
}


NTSTATUS OnPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourceList,
    _In_ WDFCMRESLIST ResourceListTranslated)
{ 
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);

    NTSTATUS status = SpbInitialize(pDevContext, ResourceList, ResourceListTranslated);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"OnPrepareHardware SpbInitialize failed", NULL, status);
        return status;
    }

    status = AcpiInitialize(pDevContext);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"OnPrepareHardware AcpiInitialize failed", NULL, status);
        return status;
    }

    RtlZeroMemory(&pDevContext->tp_settings, sizeof(PTP_PARSER));

    pDevContext->CONTACT_COUNT_MAXIMUM = PTP_MAX_CONTACT_POINTS;
    pDevContext->PAD_TYPE = 0;
    pDevContext->INPUT_MODE = 0;
    pDevContext->FUNCTION_SWITCH = 0;

    pDevContext->REPORTID_MULTITOUCH_COLLECTION = 0;
    pDevContext->REPORTID_MOUSE_COLLECTION = 0;

    pDevContext->REPORTID_DEVICE_CAPS = 0;
    pDevContext->REPORTSIZE_DEVICE_CAPS = 0;

    pDevContext->REPORTID_INPUT_MODE = 0;
    pDevContext->REPORTSIZE_INPUT_MODE = 0;

    pDevContext->REPORTID_FUNCTION_SWITCH = 0;
    pDevContext->REPORTSIZE_FUNCTION_SWITCH = 0;

    pDevContext->REPORTID_PTPHQA = 0;
    pDevContext->REPORTSIZE_PTPHQA = 0;//

    pDevContext->HidReportDescriptorSaved = FALSE;
    
    pDevContext->bHybrid_ReportingMode = FALSE;//默认初始值为Parallel mode并行报告模式
    pDevContext->DeviceDescriptorFingerCount = 0;//描述符计算单个报告数据包手指数量

    pDevContext->HostInitiatedResetActive = FALSE;//新增

    pDevContext->FirstD0Entry = TRUE;//驱动首次加电运行

    runtimes_hid = 0;
    runtimes_OnInterruptIsr = 0;
    runtimes_OnPostInterruptsEnabled = 0;
    runtimes_OnSelfManagedIoSuspend = 0;
    runtimes_ioControl = 0;
    runtimes_HidEvtResetTimerFired = 0;
    runtimes_IOCTL_HID_READ_REPORT = 0;

    RegDebug(L"OnPrepareHardware ok", NULL, status);
    return STATUS_SUCCESS;
}

NTSTATUS OnReleaseHardware(_In_  WDFDEVICE FxDevice, _In_  WDFCMRESLIST  FxResourcesTranslated)
{
    UNREFERENCED_PARAMETER(FxResourcesTranslated);

    PDEVICE_CONTEXT pDevContext = GetDeviceContext(FxDevice);

    if (pDevContext->SpbRequest) {
        WdfDeleteObjectSafe(pDevContext->SpbRequest);
    }

    if (pDevContext->SpbIoTarget) {
        WdfDeleteObjectSafe(pDevContext->SpbIoTarget);
    }

    RegDebug(L"OnReleaseHardware ok", NULL, 0);
    return STATUS_SUCCESS;

}

NTSTATUS OnD0Entry(_In_  WDFDEVICE FxDevice, _In_  WDF_POWER_DEVICE_STATE  FxPreviousState)
{
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(FxDevice);

    NTSTATUS status = HidInitialize(pDevContext, FxPreviousState);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"OnD0Entry HidInitialize failed", NULL, status);
        return status;
    }


    if (!pDevContext->HidReportDescriptorSaved) {
        status = GetReportDescriptor(pDevContext);
        if (!NT_SUCCESS(status)) {
            RegDebug(L"GetReportDescriptor err", NULL, runtimes_ioControl);
            return status;
        }
        pDevContext->HidReportDescriptorSaved = TRUE;

        status = AnalyzeHidReportDescriptor(pDevContext);
        if (!NT_SUCCESS(status)) {
            RegDebug(L"AnalyzeHidReportDescriptor err", NULL, runtimes_ioControl);
            return status;
        }
    }

    //和硬件无关的动态变量必须在OnD0Entry加电过程中初始化否则休眠模式下唤醒后变量有可能会丢失赋值出问题
    pDevContext->bSetAAPThresholdOK = FALSE;//未设置AAPThreshold
    pDevContext->PtpInputModeOn = FALSE;
    pDevContext->SetFeatureReady = TRUE;
    pDevContext->SetInputModeOK = FALSE;
    pDevContext->SetFunSwicthOK = FALSE;
    pDevContext->GetStringStep = 0;

    RtlZeroMemory(&pDevContext->currentPartOfFrame, sizeof(HYBRID_REPORT));
    RtlZeroMemory(&pDevContext->combinedPacket, sizeof(PTP_REPORT));
    pDevContext->contactCountIndex = 0;
    pDevContext->CombinedPacketReady = FALSE;

    pDevContext->bMouseLikeTouchPad_Mode = TRUE;//默认初始值为仿鼠标触摸板操作方式

    //初始化当前登录用户的SID
    pDevContext->strCurrentUserSID.Buffer = NULL;
    pDevContext->strCurrentUserSID.Length = 0;
    pDevContext->strCurrentUserSID.MaximumLength = 0;

    pDevContext->MouseSensitivity_Index = 1;//默认初始值为MouseSensitivityTable存储表的序号1项
    pDevContext->MouseSensitivity_Value = MouseSensitivityTable[pDevContext->MouseSensitivity_Index];//默认初始值为1.0

    //读取鼠标灵敏度设置
    ULONG ms_idx;
    status = GetRegisterMouseSensitivity(pDevContext, &ms_idx);
    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)//     ((NTSTATUS)0xC0000034L)
        {
            RegDebug(L"OnPrepareHardware GetRegisterMouseSensitivity STATUS_OBJECT_NAME_NOT_FOUND", NULL, status);
            status = SetRegisterMouseSensitivity(pDevContext, pDevContext->MouseSensitivity_Index);//初始默认设置
            if (!NT_SUCCESS(status)) {
                RegDebug(L"OnPrepareHardware SetRegisterMouseSensitivity err", NULL, status);
            }
        }
        else
        {
            RegDebug(L"OnPrepareHardware GetRegisterMouseSensitivity err", NULL, status);
        }
    }
    else {
        if (ms_idx > 2) {//如果读取的数值错误
            ms_idx = pDevContext->MouseSensitivity_Index;//恢复初始默认值
        }
        pDevContext->MouseSensitivity_Index = (UCHAR)ms_idx;
        pDevContext->MouseSensitivity_Value = MouseSensitivityTable[pDevContext->MouseSensitivity_Index];
        RegDebug(L"OnPrepareHardware GetRegisterMouseSensitivity MouseSensitivity_Index=", NULL, pDevContext->MouseSensitivity_Index);
    }


    MouseLikeTouchPad_parse_init(pDevContext);

    PowerIdleIrpCompletion(pDevContext);//

    RegDebug(L"OnD0Entry ok", NULL, 0);
    return STATUS_SUCCESS;
}

NTSTATUS OnD0Exit(_In_ WDFDEVICE FxDevice, _In_ WDF_POWER_DEVICE_STATE FxPreviousState)
{
    PAGED_CODE();

    PDEVICE_CONTEXT pDevContext = GetDeviceContext(FxDevice);
    NT_ASSERT(pDevContext != NULL);

    NTSTATUS status = HidDestroy(pDevContext, FxPreviousState);

    RegDebug(L"OnD0Exit ok", NULL, 0);
    return status;
}



NTSTATUS
SpbInitialize(
    _In_ PDEVICE_CONTEXT    FxDeviceContext,
    _In_ WDFCMRESLIST       FxResourcesRaw,
    _In_ WDFCMRESLIST       FxResourcesTranslated
)
{

    UNREFERENCED_PARAMETER(FxResourcesRaw);

    NTSTATUS status = STATUS_SUCCESS;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor = NULL;

    ULONG interruptIndex = 0;

    {
        BOOLEAN connFound = FALSE;
        BOOLEAN interruptFound = FALSE;

        ULONG resourceCount = WdfCmResourceListGetCount(FxResourcesTranslated);

        for (ULONG i = 0; ((connFound == FALSE) || (interruptFound == FALSE)) && (i < resourceCount); i++)
        {
            pDescriptor = WdfCmResourceListGetDescriptor(FxResourcesTranslated, i);

            NT_ASSERTMSG("Resource descriptor is NULL", pDescriptor != NULL);

            switch (pDescriptor->Type)
            {
            case CmResourceTypeConnection:
                if (pDescriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&//测试CM_RESOURCE_CONNECTION_CLASS_GPIO//CM_RESOURCE_CONNECTION_CLASS_SERIAL
                    pDescriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C)//测试CM_RESOURCE_CONNECTION_TYPE_GPIO_IO//CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C
                {
                    FxDeviceContext->SpbConnectionId.LowPart = pDescriptor->u.Connection.IdLowPart;
                    FxDeviceContext->SpbConnectionId.HighPart = pDescriptor->u.Connection.IdHighPart;

                    connFound = TRUE;
                    RegDebug(L"I2C resource found with connection id:", NULL, FxDeviceContext->SpbConnectionId.LowPart);
                }
                
                if ((pDescriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_GPIO) &&
                    (pDescriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_GPIO_IO))
                {

                    FxDeviceContext->SpbConnectionId.LowPart = pDescriptor->u.Connection.IdLowPart;
                    FxDeviceContext->SpbConnectionId.HighPart = pDescriptor->u.Connection.IdHighPart;
                    connFound = TRUE;
                    RegDebug(L"I2C GPIO resource found with connection id:", NULL, FxDeviceContext->SpbConnectionId.LowPart);
                }
                break;

            case CmResourceTypeInterrupt:
                interruptFound = TRUE;
                interruptIndex = i;

                RegDebug(L"Interrupt resource found at index:", NULL, interruptIndex);

                break;

            default:
                break;
            }
        }


        if ((connFound == FALSE) || (interruptFound == FALSE))
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            RegDebug(L"Failed finding required resources", NULL, status);

            goto exit;
        }
    }


    //
    {
        WDF_OBJECT_ATTRIBUTES targetAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&targetAttributes);

        status = WdfIoTargetCreate(
            FxDeviceContext->FxDevice,
            &targetAttributes,
            &FxDeviceContext->SpbIoTarget);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"WdfIoTargetCreate failed creating SPB target", NULL, status);

            WdfDeleteObjectSafe(FxDeviceContext->SpbIoTarget);
            goto exit;
        }

        //
        DECLARE_UNICODE_STRING_SIZE(spbDevicePath, RESOURCE_HUB_PATH_SIZE);

        status = RESOURCE_HUB_CREATE_PATH_FROM_ID(
            &spbDevicePath,
            FxDeviceContext->SpbConnectionId.LowPart,
            FxDeviceContext->SpbConnectionId.HighPart);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"ResourceHub failed to create device path", NULL, status);

            goto exit;
        }


        WDF_IO_TARGET_OPEN_PARAMS  openParams;
        WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
            &openParams,
            &spbDevicePath,
            (GENERIC_READ | GENERIC_WRITE));

        openParams.ShareAccess = 0;
        openParams.CreateDisposition = FILE_OPEN;
        openParams.FileAttributes = FILE_ATTRIBUTE_NORMAL;

        status = WdfIoTargetOpen(
            FxDeviceContext->SpbIoTarget,
            &openParams);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"WdfIoTargetOpen failed to open SPB target", NULL, status);

            goto exit;
        }
    }

    //
    {

        WDF_OBJECT_ATTRIBUTES requestAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, REQUEST_CONTEXT);

        requestAttributes.ParentObject = FxDeviceContext->SpbIoTarget;

        status = WdfRequestCreate(
            &requestAttributes,
            FxDeviceContext->SpbIoTarget,
            &FxDeviceContext->SpbRequest);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"WdfRequestCreate failed creating SPB request", NULL, status);

            goto exit;
        }
        //
        // Initialize the request context with default values
        //
        PREQUEST_CONTEXT pRequestContext = GetRequestContext(FxDeviceContext->SpbRequest);
        pRequestContext->FxDevice = FxDeviceContext->FxDevice;
        pRequestContext->FxMemory = NULL;
    }

    //
    WDF_OBJECT_ATTRIBUTES lockAttributes;

    {
        WDFWAITLOCK interruptLock;
        WDF_OBJECT_ATTRIBUTES_INIT(&lockAttributes);

 
        lockAttributes.ParentObject = FxDeviceContext->FxDevice;

        status = WdfWaitLockCreate(
            &lockAttributes,
            &interruptLock);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"WdfWaitLockCreate failed creating interrupt lock", NULL, status);

            goto exit;
        }
        

        WDF_INTERRUPT_CONFIG interruptConfig;

        WDF_INTERRUPT_CONFIG_INIT(
            &interruptConfig,
            OnInterruptIsr,
            NULL);

        interruptConfig.PassiveHandling = TRUE;
        interruptConfig.WaitLock = interruptLock;
        interruptConfig.InterruptTranslated = WdfCmResourceListGetDescriptor(FxResourcesTranslated, interruptIndex);
        interruptConfig.InterruptRaw = WdfCmResourceListGetDescriptor(FxResourcesRaw, interruptIndex);

        //interruptConfig.EvtInterruptDpc = OnInterruptDpc;

        status = WdfInterruptCreate(
            FxDeviceContext->FxDevice,
            &interruptConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &FxDeviceContext->ReadInterrupt);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"WdfInterruptCreate failed creating interrupt", NULL, status);

            goto exit;
        }
    }

    //
    {
        WDF_OBJECT_ATTRIBUTES_INIT(&lockAttributes);
        lockAttributes.ParentObject = FxDeviceContext->FxDevice;;

        status = WdfSpinLockCreate(
            &lockAttributes,
            &FxDeviceContext->InProgressLock);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"WdfSpinLockCreate failed creating inprogress spinlock", NULL, status);

            goto exit;
        }
    }

    RegDebug(L"SpbInitialize ok", NULL, status);

exit:

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"EventWrite_HIDI2C_ENUMERATION_SPB_FAILURE", NULL, status);
    }

    //RegDebug(L"SpbInitialize end", NULL, status);
    return status;
}

VOID
SpbDestroy(
    _In_  PDEVICE_CONTEXT  FxDeviceContext
)
{

    WdfDeleteObjectSafe(FxDeviceContext->SpbRequest);
    WdfDeleteObjectSafe(FxDeviceContext->SpbIoTarget);

    RegDebug(L"WdfObjectDelete closed and deleted SpbIoTarget", NULL, 0);

    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SpbWrite(
    _In_                    WDFIOTARGET FxIoTarget,
    _In_                    USHORT      RegisterAddress,
    _In_reads_(DataLength)  PBYTE       Data,
    _In_                    ULONG       DataLength,
    _In_                    ULONG       Timeout
)
{
    NTSTATUS status;

    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    WDFMEMORY memoryWrite = NULL;

    if (Data == NULL || DataLength <= 0)
    {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"SpbWrite failed parameters DataLength", NULL, status);

        goto exit;
    }

    //
    PBYTE pBuffer;
    ULONG bufferLength = REG_SIZE + DataLength;

    status = WdfMemoryCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        NonPagedPoolNx,
        HIDI2C_POOL_TAG,
        bufferLength,
        &memoryWrite,
        (PVOID*)&pBuffer);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"WdfMemoryCreate failed allocating memory buffer for SpbWrite", NULL, status);

        goto exit;
    }

    WDF_MEMORY_DESCRIPTOR  memoryDescriptor;
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(
        &memoryDescriptor,
        memoryWrite,
        NULL);


    RtlCopyMemory(pBuffer, &RegisterAddress, REG_SIZE);
    RtlCopyMemory((pBuffer + REG_SIZE), Data, DataLength);


    ULONG_PTR bytesWritten;

    if (Timeout == 0)
    {
        status = WdfIoTargetSendWriteSynchronously(
            FxIoTarget,
            NULL,
            &memoryDescriptor,
            NULL,
            NULL,
            &bytesWritten);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"WdfIoTargetSendWriteSynchronously failed sending SpbWrite without a timeout", NULL, status);

            goto exit;
        }
    }
    else
    {

        WDF_REQUEST_SEND_OPTIONS sendOptions;
        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
        sendOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(Timeout);////原先调用者的单位为秒太巨大改为ms

        status = WdfIoTargetSendWriteSynchronously(
            FxIoTarget,
            NULL,
            &memoryDescriptor,
            NULL,
            &sendOptions,
            &bytesWritten);

        if (!NT_SUCCESS(status))
        {
            RegDebug(L"WdfIoTargetSendWriteSynchronously failed sending SpbWrite with timeout", NULL, status);

            goto exit;
        }
    }
    //
    ULONG expectedLength = REG_SIZE + DataLength;
    if (bytesWritten != expectedLength)
    {
        status = STATUS_DEVICE_PROTOCOL_ERROR;
        RegDebug(L"WdfIoTargetSendWriteSynchronously returned with bytes expected", NULL, status);

        goto exit;
    }

exit:

    //RegDebug(L"SpbWrite end", NULL, status);
    WdfDeleteObjectSafe(memoryWrite);
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SpbWritelessRead(
    _In_                        WDFIOTARGET FxIoTarget,
    _In_                        WDFREQUEST  FxRequest,
    _Out_writes_(DataLength)    PBYTE       Data,
    _In_                        ULONG       DataLength
)
{
    NTSTATUS status = STATUS_SUCCESS;

    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (Data == NULL || DataLength <= 0)
    {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"SpbWritelessRead failed parameters DataLength", NULL, status);

        goto exit;
    }

    PREQUEST_CONTEXT pRequestContext = GetRequestContext(FxRequest);
    WDFMEMORY* pInputReportMemory = &pRequestContext->FxMemory;

    status = WdfMemoryAssignBuffer(
        *pInputReportMemory,
        Data,
        DataLength);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"WdfMemoryAssignBuffer failed assigning input report buffer", NULL, status);
        goto exit;
    }

    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(
        &memoryDescriptor,
        *pInputReportMemory,
        NULL);


    WDF_REQUEST_REUSE_PARAMS    reuseParams;
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    WdfRequestReuse(FxRequest, &reuseParams);

    WDF_REQUEST_SEND_OPTIONS sendOptions;
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
    sendOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(HIDI2C_REQUEST_DEFAULT_TIMEOUT);//原先单位为秒太大改为ms

    //
    ULONG_PTR bytesRead = 0;
    status = WdfIoTargetSendReadSynchronously(
        FxIoTarget,
        FxRequest,
        &memoryDescriptor,
        NULL,
        &sendOptions,
        &bytesRead);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"Failed sending SPB Read bytes", NULL, status);
    }


exit:

    //RegDebug(L"SpbWritelessRead end", NULL, status);
    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SpbRead(
    _In_                        WDFIOTARGET FxIoTarget,
    _In_                        USHORT      RegisterAddress,
    _Out_writes_(DataLength)    PBYTE       Data,
    _In_                        ULONG       DataLength,
    _In_                        ULONG       DelayUs,
    _In_                        ULONG       Timeout
)
{

    NTSTATUS status;

    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (Data == NULL || DataLength <= 0)
    {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"SpbRead failed parameters DataLength", NULL, status);

        goto exit;
    }

    const ULONG transfers = 2;
    SPB_TRANSFER_LIST_AND_ENTRIES(transfers)    sequence;
    SPB_TRANSFER_LIST_INIT(&(sequence.List), transfers);

    {
        ULONG index = 0;
        sequence.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
            SpbTransferDirectionToDevice,
            0,
            &RegisterAddress,
            REG_SIZE);

        sequence.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
            SpbTransferDirectionFromDevice,
            DelayUs,
            Data,
            DataLength);
    }

    //
    ULONG bytesReturned = 0;
    status = ::SpbSequence(FxIoTarget, &sequence, sizeof(sequence), &bytesReturned, Timeout);
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"SpbSequence failed sending a sequence", NULL, status);
        goto exit;
    }

    //
    ULONG expectedLength = REG_SIZE;
    if (bytesReturned < expectedLength)
    {
        status = STATUS_DEVICE_PROTOCOL_ERROR;
        RegDebug(L"SpbSequence returned with  bytes expected", NULL, status);
    }

exit:

    //RegDebug(L"SpbRead end", NULL, status);
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SpbWriteWrite(
    _In_                            WDFIOTARGET FxIoTarget,
    _In_                            USHORT      RegisterAddressFirst,
    _In_reads_(DataLengthFirst)     PBYTE       DataFirst,
    _In_                            USHORT      DataLengthFirst,
    _In_                            USHORT      RegisterAddressSecond,
    _In_reads_(DataLengthSecond)    PBYTE       DataSecond,
    _In_                            USHORT      DataLengthSecond
)
{
    NTSTATUS status;

    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (DataFirst == NULL || DataLengthFirst <= 0 ||
        DataSecond == NULL || DataLengthSecond <= 0)
    {
        status = STATUS_INVALID_PARAMETER;
        //RegDebug(L"SpbWriteWrite failed parameters DataFirst DataLengthFirst", NULL, status);

        goto exit;
    }


    const ULONG bufferListEntries = 4;
    SPB_TRANSFER_BUFFER_LIST_ENTRY BufferListFirst[bufferListEntries];
    BufferListFirst[0].Buffer = &RegisterAddressFirst;
    BufferListFirst[0].BufferCb = REG_SIZE;
    BufferListFirst[1].Buffer = DataFirst;
    BufferListFirst[1].BufferCb = DataLengthFirst;
    BufferListFirst[2].Buffer = &RegisterAddressSecond;
    BufferListFirst[2].BufferCb = REG_SIZE;
    BufferListFirst[3].Buffer = DataSecond;
    BufferListFirst[3].BufferCb = DataLengthSecond;

    const ULONG transfers = 1;
    SPB_TRANSFER_LIST sequence;
    SPB_TRANSFER_LIST_INIT(&sequence, transfers);

    {
        sequence.Transfers[0] = SPB_TRANSFER_LIST_ENTRY_INIT_BUFFER_LIST(
            SpbTransferDirectionToDevice,   // Transfer Direction
            0,                              // Delay (1st write has no delay)
            BufferListFirst,                // Pointer to buffer
            bufferListEntries);             // Length of buffer
    }

    ULONG bytesReturned = 0;
    status = ::SpbSequence(FxIoTarget, &sequence, sizeof(sequence), &bytesReturned, HIDI2C_REQUEST_DEFAULT_TIMEOUT);//原先单位为秒太大改为ms

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"SpbSequence failed sending a sequence", NULL, status);

        goto exit;
    }

    //
    ULONG expectedLength = REG_SIZE + DataLengthFirst + REG_SIZE + DataLengthSecond;
    if (bytesReturned != expectedLength)
    {
        status = STATUS_DEVICE_PROTOCOL_ERROR;
        RegDebug(L"SpbSequence returned with  bytes expected", NULL, status);

        goto exit;
    }

exit:

    //RegDebug(L"SpbWriteWrite end", NULL, status);
    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SpbWriteRead(
    _In_                            WDFIOTARGET     FxIoTarget,
    _In_                            USHORT          RegisterAddressFirst,
    _In_reads_(DataLengthFirst)     PBYTE           DataFirst,
    _In_                            USHORT          DataLengthFirst,
    _In_                            USHORT          RegisterAddressSecond,
    _Out_writes_(DataLengthSecond)  PBYTE           DataSecond,
    _In_                            USHORT          DataLengthSecond,
    _In_                            ULONG           DelayUs
)
{
    NTSTATUS status;

    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (DataFirst == NULL || DataLengthFirst <= 0 ||
        DataSecond == NULL || DataLengthSecond <= 0)
    {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"SpbWriteRead failed parameters DataFirst DataLengthFirst", NULL, status);

        goto exit;
    }

    //
    const ULONG bufferListEntries = 3;
    SPB_TRANSFER_BUFFER_LIST_ENTRY BufferListFirst[bufferListEntries];
    BufferListFirst[0].Buffer = &RegisterAddressFirst;
    BufferListFirst[0].BufferCb = REG_SIZE;
    BufferListFirst[1].Buffer = DataFirst;
    BufferListFirst[1].BufferCb = DataLengthFirst;
    BufferListFirst[2].Buffer = &RegisterAddressSecond;
    BufferListFirst[2].BufferCb = REG_SIZE;

    const ULONG transfers = 2;
    SPB_TRANSFER_LIST_AND_ENTRIES(transfers)    sequence;
    SPB_TRANSFER_LIST_INIT(&(sequence.List), transfers);

    {
        ULONG index = 0;

        sequence.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_BUFFER_LIST(
            SpbTransferDirectionToDevice,
            0,
            BufferListFirst,
            bufferListEntries);

        sequence.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
            SpbTransferDirectionFromDevice,
            DelayUs,
            DataSecond,
            DataLengthSecond);
    }

    ULONG bytesReturned = 0;
    status = ::SpbSequence(FxIoTarget, &sequence, sizeof(sequence), &bytesReturned, HIDI2C_REQUEST_DEFAULT_TIMEOUT);//原先单位为秒太大改为ms

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"SpbSequence failed sending a sequence", NULL, status);

        goto exit;
    }

    //
    ULONG expectedLength = REG_SIZE + DataLengthFirst + REG_SIZE;
    if (bytesReturned < expectedLength)
    {
        status = STATUS_DEVICE_PROTOCOL_ERROR;
        RegDebug(L"SpbSequence returned with bytes expected", NULL, status);
    }

exit:

    //RegDebug(L"SpbWriteRead end", NULL, status);
    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SpbSequence(
    _In_                        WDFIOTARGET FxIoTarget,
    _In_reads_(SequenceLength)  PVOID       Sequence,
    _In_                        SIZE_T      SequenceLength,
    _Out_                       PULONG      BytesReturned,
    _In_                        ULONG       Timeout
)
{

    NTSTATUS status;

    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
  
    WDFMEMORY memorySequence = NULL;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    status = WdfMemoryCreatePreallocated(
        &attributes,
        Sequence,
        SequenceLength,
        &memorySequence);

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"WdfMemoryCreatePreallocated failed creating memory for sequence", NULL, status);
        goto exit;
    }


    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(
        &memoryDescriptor,
        memorySequence,
        NULL);

    ULONG_PTR bytes = 0;

    if (Timeout == 0)
    {
        status = WdfIoTargetSendIoctlSynchronously(
            FxIoTarget,
            NULL,
            IOCTL_SPB_EXECUTE_SEQUENCE,
            &memoryDescriptor,
            NULL,
            NULL,
            &bytes);
    }
    else
    {
        WDF_REQUEST_SEND_OPTIONS sendOptions;
        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
        sendOptions.Timeout = WDF_REL_TIMEOUT_IN_MS(Timeout);//////原先调用者的单位为秒太巨大改为ms

        status = WdfIoTargetSendIoctlSynchronously(
            FxIoTarget,
            NULL,
            IOCTL_SPB_EXECUTE_SEQUENCE,
            &memoryDescriptor,
            NULL,
            &sendOptions,
            &bytes);
    }
  
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"Failed sending SPB Sequence IOCTL bytes", NULL, status);

        goto exit;
    }

    *BytesReturned = (ULONG)bytes;

exit:

    //RegDebug(L"_SpbSequence end", NULL, status);
    WdfDeleteObjectSafe(memorySequence);
    return status;
}





NTSTATUS HidPower(PDEVICE_CONTEXT pDevContext, SHORT PowerState)
{
    //RegDebug(L"HidPower start", NULL, runtimes_hid++);

    NTSTATUS status = STATUS_SUCCESS;
    USHORT RegisterAddress;

    RegisterAddress = pDevContext->HidSettings.CommandRegisterAddress;
    WDFIOTARGET IoTarget = pDevContext->SpbIoTarget;

    USHORT state = PowerState | 0x800;
    status = SpbWrite(IoTarget, RegisterAddress, (PUCHAR)&state, 2, 5* HIDI2C_REQUEST_DEFAULT_TIMEOUT);//原先单位为秒太大改为ms
    if (!NT_SUCCESS(status)) {
        RegDebug(L"HidPower SpbWrite failed", NULL, status);
    }

    RegDebug(L"HidPower end", NULL, runtimes_hid++);
    return status;
}


NTSTATUS HidReset(PDEVICE_CONTEXT pDevContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR data[2];

    USHORT RegisterAddress = pDevContext->HidSettings.CommandRegisterAddress;
    WDFDEVICE device = pDevContext->FxDevice;

    ULONG value = 0;
    WDFKEY hKey = NULL;
    *(PUSHORT)data = 256;//0x0100

    DECLARE_CONST_UNICODE_STRING(ValueNameString, L"DoNotWaitForResetResponse");

    //伪代码很可能少了2个参数??
    status = WdfDeviceOpenRegistryKey(
        device,
        PLUGPLAY_REGKEY_DEVICE,//1
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &hKey);


        //WdfDeviceOpenDevicemapKey(
        //	device,
        //	&ValueNameString,
        //	KEY_READ,
        //	WDF_NO_OBJECT_ATTRIBUTES,
        //	&hKey);

    if (NT_SUCCESS(status)) {
        status = WdfRegistryQueryULong(hKey, &ValueNameString, &value);
        if (NT_SUCCESS(status))
        {
            RegDebug(L"HidReset WdfRegistryQueryULong ok", NULL, status);
        }
        else
        {
            if (status == STATUS_OBJECT_NAME_NOT_FOUND)// ((NTSTATUS)0xC0000034L)
            {
                RegDebug(L"HidReset WdfRegistryQueryULong STATUS_OBJECT_NAME_NOT_FOUND", NULL, status);
            }
        }
    }


    if (hKey)
        WdfObjectDelete(hKey);


    WdfInterruptAcquireLock(pDevContext->ReadInterrupt);

    if (!value)
    {      
        WdfIoQueueStopSynchronously(pDevContext->IoctlQueue);
        WdfTimerStart(pDevContext->timerHandle, WDF_REL_TIMEOUT_IN_MS(10));//400
        RegDebug(L"HidReset WdfTimerStart timerHandle", NULL, status);
    }

    status = SpbWrite(pDevContext->SpbIoTarget, RegisterAddress, data, 2u, HIDI2C_REQUEST_DEFAULT_TIMEOUT);///原先单位为秒太大改为ms
    if (NT_SUCCESS(status))
    {
        pDevContext->HostInitiatedResetActive = TRUE;
        RegDebug(L"HidReset HostInitiatedResetActive TRUE", NULL, runtimes_hid++);

    }

    WdfInterruptReleaseLock(pDevContext->ReadInterrupt);

    return status;
}

NTSTATUS HidDestroy(PDEVICE_CONTEXT pDevContext, WDF_POWER_DEVICE_STATE FxTargetState)
{

    NTSTATUS status = HidPower(pDevContext, 1);
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"HidDestroy _HidPower failed", NULL, status);
        goto exit;
    }

    if (FxTargetState != WdfPowerDeviceD3Final) {
        RegDebug(L"HidDestroy FxTargetState err", NULL, status);
        goto exit;
    }

    PVOID buffer = pDevContext->pHidInputReport;
    if (buffer) {
        ExFreePoolWithTag(buffer, HIDI2C_POOL_TAG);
        pDevContext->pHidInputReport = NULL;
    }

exit:
    RegDebug(L"HidDestroy end", NULL, status);
    return status;
}


NTSTATUS HidInitialize(PDEVICE_CONTEXT pDevContext, WDF_POWER_DEVICE_STATE  FxPreviousState)
{

    NTSTATUS status = STATUS_SUCCESS;//STATUS_UNSUCCESSFUL

    WDF_DEVICE_POWER_STATE state;
    state = WdfDeviceGetDevicePowerState(pDevContext->FxDevice);
    //RegDebug(L"HidInitialize powerstate", NULL, state);//测试为WdfDevStatePowerD0Starting

    if (FxPreviousState != WdfPowerDeviceD3Final)
    {
        status = HidPower(pDevContext, 0);
        if (!NT_SUCCESS(status))
        {
            RegDebug(L"_HidPower failed", NULL, status);
            goto exit;
        }
    }

    status = HidGetHidDescriptor(pDevContext);
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"_HidGetHidDescriptor failed", NULL, status);
        goto exit;
    }

    size_t InputReportMaxLength = pDevContext->HidSettings.InputReportMaxLength;
    PVOID buffer = ExAllocatePoolWithTag(NonPagedPoolNx, InputReportMaxLength, HIDI2C_POOL_TAG);
    pDevContext->pHidInputReport = (PBYTE)buffer;

    if (!buffer) {
        //
        //  No memory
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"HidInitialize ExAllocatePoolWithTag failed", NULL, status);
        goto exit;
    }
  

    PREQUEST_CONTEXT pRequestContext = GetRequestContext(pDevContext->SpbRequest);
    WDFMEMORY* pInputReportMemory = &pRequestContext->FxMemory;

    if (!(*pInputReportMemory)) {

        PVOID Sequence = pDevContext->pHidInputReport;
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = pDevContext->SpbRequest;

        size_t SequenceLength = InputReportMaxLength;
        status = WdfMemoryCreatePreallocated(&attributes, Sequence, SequenceLength, pInputReportMemory);
        if (!NT_SUCCESS(status))
        {
            RegDebug(L"HidInitialize WdfMemoryCreatePreallocated failed", NULL, status);
            goto exit;
        }
    }

    pDevContext->InputReportListHead.Blink = &pDevContext->InputReportListHead;
    pDevContext->InputReportListHead.Flink = &pDevContext->InputReportListHead;


    WDF_OBJECT_ATTRIBUTES lockAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&lockAttributes);

    status = WdfSpinLockCreate(&lockAttributes, &pDevContext->InputReportListLock);
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"HidInitialize WdfSpinLockCreate failed", NULL, status);
        goto exit;
    }

    RegDebug(L"HidInitialize ok", NULL, status);

exit:
    //RegDebug(L"HidInitialize end", NULL, status);
    return status;
}


NTSTATUS HidSendIdleNotification(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST FxRequest,
    BOOLEAN* bRequestPendingFlag)
{
    NTSTATUS status = STATUS_SUCCESS;
    *bRequestPendingFlag = FALSE;

    PIRP pIrp = WdfRequestWdmGetIrp(FxRequest);
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(pIrp);//即是PIO_STACK_LOCATION IoStack = Irp->Tail.Overlay.CurrentStackLocation；
    PWORKITEM_CONTEXT pWorkItemContext = (PWORKITEM_CONTEXT)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;//v6是指针因为前面有指针转换

    if (pWorkItemContext && pWorkItemContext->FxDevice) {

        //WDFDEVICE device = pDevContext->FxDevice;

        WDF_WORKITEM_CONFIG WorkItemConfig;
        WDF_WORKITEM_CONFIG_INIT(&WorkItemConfig, PowerIdleIrpWorkitem);

        WDFWORKITEM IdleWorkItem;

        WDF_OBJECT_ATTRIBUTES WorkItemAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&WorkItemAttributes);

        status = WdfWorkItemCreate(&WorkItemConfig, &WorkItemAttributes, &IdleWorkItem);
        if (NT_SUCCESS(status)) {

            PWORKITEM_CONTEXT pWorkItemContext_new = GetWorkItemContext(IdleWorkItem);
            pWorkItemContext_new->FxDevice = pDevContext->FxDevice;
            pWorkItemContext_new->FxRequest = pDevContext->SpbRequest;

            WdfWorkItemEnqueue(IdleWorkItem);

            *bRequestPendingFlag = TRUE;
        }
    }
    else
    {
        status = STATUS_NO_CALLBACK_ACTIVE;
        RegDebug(L"HidSendIdleNotification STATUS_NO_CALLBACK_ACTIVE", NULL, status);
    }

    return status;
}


NTSTATUS HidGetHidDescriptor(PDEVICE_CONTEXT pDevContext)
{

    NTSTATUS status = STATUS_SUCCESS;
    USHORT RegisterAddress = pDevContext->AcpiSettings.HidDescriptorAddress;
    PBYTE pHidDescriptorLength = (PBYTE)&pDevContext->HidSettings.HidDescriptorLength;////注意pHidDescriptorLength需要用指针，因为后SpbRead后被赋值改变了后续*pHidDescriptorLength调用需要
    pDevContext->HidSettings.HidDescriptorLength = 0;
    pDevContext->HidSettings.InputRegisterAddress = NULL;
    pDevContext->HidSettings.CommandRegisterAddress = NULL;

    pDevContext->HidSettings.VersionId = 0;
    pDevContext->HidSettings.Reserved = 0;

    ULONG DelayUs = 0;
    status = SpbRead(pDevContext->SpbIoTarget, RegisterAddress, (PBYTE)&pDevContext->HidSettings.HidDescriptorLength, 0x1Eu, DelayUs, 0);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"HidGetHidDescriptor SpbRead failed", NULL, status);
        return status;
    }
    //RegDebug(L"HidGetHidDescriptor SpbRead data=", pHidDescriptorLength, 0x1e);

   /* RegDebug(L"_HidGetHidDescriptor SpbRead HidDescriptorLength=", NULL, pDevContext->HidSettings.HidDescriptorLength);
    RegDebug(L"_HidGetHidDescriptor SpbRead BcdVersion=", NULL, pDevContext->HidSettings.BcdVersion);
    RegDebug(L"_HidGetHidDescriptor SpbRead ReportDescriptorLength=", NULL, pDevContext->HidSettings.ReportDescriptorLength);
    RegDebug(L"_HidGetHidDescriptor SpbRead ReportDescriptorAddress=", NULL, pDevContext->HidSettings.ReportDescriptorAddress);
    RegDebug(L"_HidGetHidDescriptor SpbRead InputRegisterAddress=", NULL, pDevContext->HidSettings.InputRegisterAddress);
    RegDebug(L"_HidGetHidDescriptor SpbRead InputReportMaxLength=", NULL, pDevContext->HidSettings.InputReportMaxLength);
    RegDebug(L"_HidGetHidDescriptor SpbRead OutputRegisterAddress=", NULL, pDevContext->HidSettings.OutputRegisterAddress);
    RegDebug(L"_HidGetHidDescriptor SpbRead OutputReportMaxLength=", NULL, pDevContext->HidSettings.OutputReportMaxLength);
    RegDebug(L"_HidGetHidDescriptor SpbRead CommandRegisterAddress=", NULL, pDevContext->HidSettings.CommandRegisterAddress);
    RegDebug(L"_HidGetHidDescriptor SpbRead DataRegisterAddress=", NULL, pDevContext->HidSettings.DataRegisterAddress);
    RegDebug(L"_HidGetHidDescriptor SpbRead VendorId=", NULL, pDevContext->HidSettings.VendorId);
    RegDebug(L"_HidGetHidDescriptor SpbRead ProductId=", NULL, pDevContext->HidSettings.ProductId);
    RegDebug(L"_HidGetHidDescriptor SpbRead VersionId=", NULL, pDevContext->HidSettings.VersionId);
    RegDebug(L"_HidGetHidDescriptor SpbRead Reserved=", NULL, pDevContext->HidSettings.Reserved);*/


    if (*pHidDescriptorLength != 30//pDeviceContext->HidSettings.HidDescriptorLength!=30
        || pDevContext->HidSettings.BcdVersion != 256
        || !pDevContext->HidSettings.ReportDescriptorAddress
        || !pDevContext->HidSettings.InputRegisterAddress
        || pDevContext->HidSettings.InputReportMaxLength < 2
        || !pDevContext->HidSettings.CommandRegisterAddress
        || !pDevContext->HidSettings.DataRegisterAddress
        || !pDevContext->HidSettings.VendorId
        || pDevContext->HidSettings.Reserved)
    {

        status = STATUS_DEVICE_PROTOCOL_ERROR;
        RegDebug(L"_HidGetHidDescriptor STATUS_DEVICE_PROTOCOL_ERROR", NULL, status);
    }

    //RegDebug(L"_HidGetHidDescriptor end", NULL, status);
    return status;
}


NTSTATUS
HidGetDeviceAttributes(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PHID_DEVICE_ATTRIBUTES pDeviceAttributes = NULL;

    status = WdfRequestRetrieveOutputBuffer(
        Request,
        sizeof(HID_DEVICE_ATTRIBUTES),
        (PVOID*)&pDeviceAttributes,
        NULL
    );

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"HidGetDeviceAttributes WdfRequestRetrieveOutputBuffer failed", NULL, status);
        goto exit;
    }

    pDeviceAttributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
    pDeviceAttributes->VendorID = pDevContext->HidSettings.VendorId;
    pDeviceAttributes->ProductID = pDevContext->HidSettings.ProductId;
    pDeviceAttributes->VersionNumber = pDevContext->HidSettings.VersionId;

    WdfRequestSetInformation(
        Request,
        sizeof(HID_DEVICE_ATTRIBUTES)
    );

exit:
    RegDebug(L"HidGetDeviceAttributes end", NULL, status);
    return status;
}


NTSTATUS
HidGetDeviceDescriptor(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PHID_DESCRIPTOR pHidDescriptor = NULL;

    status = WdfRequestRetrieveOutputBuffer(
        Request,
        sizeof(PHID_DESCRIPTOR),
        (PVOID*)&pHidDescriptor,
        NULL
    );

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"HidGetDeviceDescriptor WdfRequestRetrieveOutputBuffer failed", NULL, status);
        goto exit;
    }

    USHORT ReportLength = pDevContext->HidSettings.ReportDescriptorLength;

    pHidDescriptor->bLength = sizeof(HID_DESCRIPTOR);//0x9  //pDevContext->HidSettings.HidDescriptorLength;
    pHidDescriptor->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;// 0x21;//HID_DESCRIPTOR_SIZE_V1=0x1E
    pHidDescriptor->bcdHID = HID_DESCRIPTOR_BCD_VERSION;// 0x0100;
    pHidDescriptor->bCountry = 0x00;//country code == Not Specified
    pHidDescriptor->bNumDescriptors = 0x01;

    pHidDescriptor->DescriptorList->bReportType = HID_REPORT_DESCRIPTOR_TYPE;// 0x22;
    pHidDescriptor->DescriptorList->wReportLength = ReportLength;


    WdfRequestSetInformation(
        Request,
        sizeof(HID_DESCRIPTOR)
    );

exit:
    RegDebug(L"HidGetDeviceDescriptor end", NULL, status);
    return status;
}


NTSTATUS
HidGetReportDescriptor(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request
)
{
    RegDebug(L"HidGetReportDescriptor start", NULL, runtimes_ioControl);

    NTSTATUS status = STATUS_SUCCESS;
    WDFMEMORY RequestMemory;

    status = WdfRequestRetrieveOutputMemory(Request, &RequestMemory);
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"HidGetReportDescriptor WdfRequestRetrieveOutputBuffer failed", NULL, status);
        goto exit;
    }

    USHORT RegisterAddress = pDevContext->HidSettings.ReportDescriptorAddress;
    USHORT ReportLength = pDevContext->HidSettings.ReportDescriptorLength;

    PBYTE pReportDesciptorData = (PBYTE)WdfMemoryGetBuffer(RequestMemory, NULL);
    if (!pReportDesciptorData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"HidGetReportDescriptor WdfMemoryGetBuffer failed", NULL, status);
        goto exit;
    }

    ULONG DelayUs = 0;
    status = SpbRead(pDevContext->SpbIoTarget, RegisterAddress, pReportDesciptorData, ReportLength, DelayUs, HIDI2C_REQUEST_DEFAULT_TIMEOUT);///原先单位为秒太大改为ms
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"HidGetReportDescriptor SpbRead failed", NULL, status);
        goto exit;
    }
    RegDebug(L"HidGetReportDescriptor pReportDesciptorData=", pReportDesciptorData, ReportLength);

    WdfRequestSetInformation(
        Request,
        ReportLength
    );

exit:
    RegDebug(L"HidGetReportDescriptor end", NULL, status);
    return status;
}


NTSTATUS
HidGetString(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request
)
{
    RegDebug(L"HidGetString start", NULL, runtimes_ioControl);
    UNREFERENCED_PARAMETER(pDevContext);

    NTSTATUS status = STATUS_SUCCESS;

    PIRP pIrp = WdfRequestWdmGetIrp(Request);

    PIO_STACK_LOCATION IoStack= IoGetCurrentIrpStackLocation(pIrp);//即是PIO_STACK_LOCATION IoStack = Irp->Tail.Overlay.CurrentStackLocation；
    

    USHORT stringSizeCb = 0;
    PWSTR string;

    //LONG dw = *(PULONG)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;//注意这个Type3InputBuffer读取会蓝屏，所以需要测试得出实际wStrID调用顺序
    //USHORT wStrID = LOWORD(dw);//
    //RegDebug(L"HidGetString: wStrID=", NULL, wStrID);

    //switch (wStrID) {
    //case HID_STRING_ID_IMANUFACTURER:
    //    stringSizeCb = sizeof(MANUFACTURER_ID_STRING);
    //    string = MANUFACTURER_ID_STRING;
    //    break;
    //case HID_STRING_ID_IPRODUCT:
    //    stringSizeCb = sizeof(PRODUCT_ID_STRING);
    //    string = PRODUCT_ID_STRING;
    //    break;
    //case HID_STRING_ID_ISERIALNUMBER:
    //    stringSizeCb = sizeof(SERIAL_NUMBER_STRING);
    //    string = SERIAL_NUMBER_STRING;
    //    break;
    //default:
    //    status = STATUS_INVALID_PARAMETER;
    //    RegDebug(L"HidGetString: unkown string id", NULL, 0);
    //    goto exit;
    //}

    PUCHAR step = &pDevContext->GetStringStep;
    if (*step == 0) {
        *step = 1;
    }

    if (*step == 1) {// case HID_STRING_ID_IMANUFACTURER:
          (*step)++;
          stringSizeCb = sizeof(MANUFACTURER_ID_STRING);
          string = MANUFACTURER_ID_STRING;
          //RegDebug(L"HidGetString: HID_STRING_ID_IMANUFACTURER", string, stringSizeCb*2+2);
    }
    else if (*step == 2) {//case HID_STRING_ID_IPRODUCT:
        (*step)++;
         stringSizeCb = sizeof(PRODUCT_ID_STRING);
         string = PRODUCT_ID_STRING;
         //egDebug(L"HidGetString: HID_STRING_ID_IPRODUCT", string, stringSizeCb * 2 + 2);
    }
    else if (*step == 3) {//case HID_STRING_ID_ISERIALNUMBER:
        (*step)++;
         stringSizeCb = sizeof(SERIAL_NUMBER_STRING);
         string = SERIAL_NUMBER_STRING;
         //RegDebug(L"HidGetString: HID_STRING_ID_ISERIALNUMBER", string, stringSizeCb * 2 + 2);
    }
    else{
         status = STATUS_INVALID_PARAMETER;
         RegDebug(L"HidGetString: unkown string id", NULL, 0);
         goto exit;
    }
    

    ULONG bufferlength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    RegDebug(L"HidGetString: bufferlength=", NULL, bufferlength);
    int i = -1;
    do {
        ++i;
    } while (string[i]);

    stringSizeCb = (USHORT)(2 * i + 2);

    if (stringSizeCb > bufferlength)
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        RegDebug(L"HidGetString STATUS_INVALID_BUFFER_SIZE", NULL, status);
        goto exit;
    }

    RtlMoveMemory(pIrp->UserBuffer, string, stringSizeCb);
    pIrp->IoStatus.Information = stringSizeCb;

exit:
    RegDebug(L"HidGetString end", NULL, status);
    return status;
}

NTSTATUS
HidWriteReport(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PHID_XFER_PACKET pHidPacket;
    size_t reportBufferLen;
    USHORT OutputReportMaxLength;
    USHORT OutputReportLength;
    USHORT RegisterAddress;
    PBYTE pReportData = NULL;

    WDF_REQUEST_PARAMETERS RequestParameters;
    WDF_REQUEST_PARAMETERS_INIT(&RequestParameters);
    WdfRequestGetParameters(Request, &RequestParameters);

    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength >= sizeof(HID_XFER_PACKET)) {
        pHidPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;
        if (pHidPacket) {
            reportBufferLen = pHidPacket->reportBufferLen;
            if (reportBufferLen) {
                OutputReportMaxLength = pDevContext->HidSettings.OutputReportMaxLength;
                OutputReportLength = (USHORT)reportBufferLen + 2;
                RegisterAddress = pDevContext->HidSettings.OutputRegisterAddress;
                if (OutputReportLength > OutputReportMaxLength) {
                    status = STATUS_INVALID_PARAMETER;
                    RegDebug(L"HidWriteReport OutputReportLength STATUS_INVALID_PARAMETER", NULL, status);
                    goto exit;
                }

                PBYTE pReportDesciptorData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, OutputReportLength, HIDI2C_POOL_TAG);
                pReportData = pReportDesciptorData;
                if (!pReportDesciptorData) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    RegDebug(L"HidWriteReport ExAllocatePoolWithTag failed", NULL, status);
                    goto exit;
                }

                RtlZeroMemory(pReportDesciptorData, OutputReportLength);
                *(PUSHORT)pReportData = (USHORT)reportBufferLen + 2;
                RtlMoveMemory(pReportData + 2, *(const void**)pHidPacket, reportBufferLen);

                status = SpbWrite(pDevContext->SpbIoTarget, RegisterAddress, pReportData, OutputReportLength, HIDI2C_REQUEST_DEFAULT_TIMEOUT);///原先为秒太大改为ms
                if (!NT_SUCCESS(status))
                {
                    RegDebug(L"HidWriteReport SpbWrite failed", NULL, status);
                    goto exit;
                }

                WdfRequestSetInformation(Request, reportBufferLen);
            }
            else {
                status = STATUS_BUFFER_TOO_SMALL;
                RegDebug(L"HidWriteReport STATUS_BUFFER_TOO_SMALL", NULL, status);
            }
        }
        else {
            status = STATUS_INVALID_BUFFER_SIZE;
            RegDebug(L"HidWriteReport STATUS_INVALID_BUFFER_SIZE", NULL, status);
        }
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"HidWriteReport STATUS_INVALID_PARAMETER", NULL, status);
    }

exit:
    if (pReportData) {
        ExFreePoolWithTag(pReportData, HIDI2C_POOL_TAG);
    }

    RegDebug(L"HidWriteReport end", NULL, status);
    return status;
}

NTSTATUS
HidSendResetNotification(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request,
    BOOLEAN* requestPendingFlag_reset
)
{
    NTSTATUS status = STATUS_SUCCESS;
    *requestPendingFlag_reset = FALSE;

    status = WdfRequestForwardToIoQueue(Request, pDevContext->ResetNotificationQueue);
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"HidSendResetNotification WdfRequestForwardToIoQueue failed", NULL, status);
        goto exit;
    }

    *requestPendingFlag_reset = TRUE;

exit:
    RegDebug(L"HidSendResetNotification end", NULL, status);
    return status;
}


NTSTATUS HidReadReport(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request,
    BOOLEAN* requestPendingFlag
)
{
    NTSTATUS status = STATUS_SUCCESS;
    *requestPendingFlag = FALSE;

    status = WdfRequestForwardToIoQueue(Request, pDevContext->ReportQueue);
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"HidReadReport WdfRequestForwardToIoQueue failed", NULL, status);
        goto exit;
    }

    *requestPendingFlag = TRUE;

exit:
    RegDebug(L"HidReadReport end", NULL, status);
    return status;
}


NTSTATUS
HidGetReport(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request,
    HID_REPORT_TYPE ReportType
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PHID_XFER_PACKET pHidPacket;
    size_t reportBufferLen;
    USHORT HeaderLength;
    USHORT RegisterAddressFirst;
    USHORT RegisterAddressSecond;
    PBYTE pReportData = NULL;
    PBYTE pReportDesciptorData = NULL;
    SHORT PFlag;
    SHORT mflag;
    BOOLEAN bAllocatePoolFlag = FALSE;
  

    WDF_REQUEST_PARAMETERS RequestParameters;
    WDF_REQUEST_PARAMETERS_INIT(&RequestParameters);
    WdfRequestGetParameters(Request, &RequestParameters);

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength >= sizeof(HID_XFER_PACKET)) {
        RegDebug(L"HidGetReport OutputBufferLength=", NULL, (ULONG)RequestParameters.Parameters.DeviceIoControl.OutputBufferLength);
        RegDebug(L"HidGetReport Parameters.Write.Length=", NULL, (ULONG)RequestParameters.Parameters.Write.Length);
        pHidPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;
        if (!pHidPacket) {
            status = STATUS_INVALID_PARAMETER;
            RegDebug(L"HidGetReport STATUS_INVALID_PARAMETER", NULL, status);
            goto exit;
        }
        RegDebug(L"HidGetReport pHidPacket=", pHidPacket, sizeof(PHID_XFER_PACKET));
        RegDebug(L"HidGetReport pHidPacket->reportBufferLen=", NULL, pHidPacket->reportBufferLen);
        RegDebug(L"HidGetReport pHidPacket->reportId=", NULL, pHidPacket->reportId);

        reportBufferLen = pHidPacket->reportBufferLen;
        if (reportBufferLen) {
            RegisterAddressFirst = pDevContext->HidSettings.CommandRegisterAddress;
            PFlag = 0x200;
            int Type = ReportType - 1;
            if (Type) {
                if (Type != 2) {
                    status = STATUS_INVALID_PARAMETER;
                    RegDebug(L"HidGetReport Type STATUS_INVALID_PARAMETER", NULL, status);
                    goto exit;
                }

                mflag = 0x230;
            }
            else
            {
                mflag = 0x210;
            }

            UCHAR reportId = pHidPacket->reportId;
            RegDebug(L"HidGetReport reportId=", NULL, reportId);
            if (reportId >= 0xFu) {
                HeaderLength = 3;
                PFlag = mflag | 0xF;

                PBYTE pReportDesciptorHeader = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, 3, HIDI2C_POOL_TAG);
                pReportData = pReportDesciptorHeader;
                if (!pReportDesciptorHeader) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    RegDebug(L"HidGetReport pReportDesciptorHeader STATUS_INSUFFICIENT_RESOURCES", NULL, status);
                    goto exit;
                }

                bAllocatePoolFlag = TRUE;
                *(PUSHORT)pReportDesciptorHeader = 0;
                pReportDesciptorHeader[2] = 0;
                *(PUSHORT)pReportDesciptorHeader = PFlag;
                pReportDesciptorHeader[2] = reportId;

            }
            else {
                pReportData = (PUCHAR)&PFlag;
                HeaderLength = 2;
                PFlag = mflag | reportId;
            }
            RegDebug(L"HidGetReport pReportDataHeader=", pReportData, HeaderLength);

            RegisterAddressSecond = pDevContext->HidSettings.DataRegisterAddress;
            pReportDesciptorData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, reportBufferLen + 2, HIDI2C_POOL_TAG);
            if (!pReportDesciptorData) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                RegDebug(L"HidGetReport pReportDesciptorData STATUS_INSUFFICIENT_RESOURCES", NULL, status);
                goto exit;
            }

            memset(pReportDesciptorData, 0, reportBufferLen + 2);

            ULONG DelayUs = 0;
            status = SpbWriteRead(pDevContext->SpbIoTarget, RegisterAddressFirst, pReportData, HeaderLength, RegisterAddressSecond, pReportDesciptorData, (USHORT)reportBufferLen + 2, DelayUs);
            if (NT_SUCCESS(status)) {
                size_t ReportSize = *(PUSHORT)pReportDesciptorData - 2;
                if (!ReportSize) {
                    RegDebug(L"HidGetReport ReportSize err", NULL, (ULONG)ReportSize);
                }
                else {
                    if (reportBufferLen < ReportSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        RegDebug(L"HidGetReport ReportSize STATUS_BUFFER_TOO_SMALL", NULL, status);
                        goto exit;

                    }

                    memmove(pHidPacket, pReportDesciptorData + 2, ReportSize);
                    RegDebug(L"HidGetReport pReportDesciptorData=", pReportDesciptorData, (ULONG)reportBufferLen + 2);
                    WdfRequestSetInformation(Request, ReportSize);

                }
            }
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
            RegDebug(L"HidGetReport STATUS_BUFFER_TOO_SMALL", NULL, status);
            goto exit;
        }

        if (NT_SUCCESS(status)) {
            goto exit;
        }
    }

    status = STATUS_INVALID_BUFFER_SIZE;
    RegDebug(L"HidGetReport STATUS_INVALID_BUFFER_SIZE", NULL, status);

exit:
    if (pReportDesciptorData)
        ExFreePoolWithTag(pReportDesciptorData, HIDI2C_POOL_TAG);
    if (pReportData && bAllocatePoolFlag)
        ExFreePoolWithTag(pReportData, HIDI2C_POOL_TAG);

    RegDebug(L"HidGetReport end", NULL, status);
    return status;
}


NTSTATUS
HidSetReport(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request,
    HID_REPORT_TYPE ReportType
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PHID_XFER_PACKET pHidPacket;
    size_t reportBufferLen;
    USHORT HeaderLength;
    USHORT RegisterAddressFirst;
    USHORT RegisterAddressSecond;
    PBYTE pReportData = NULL;
    PBYTE pReportDesciptorData = NULL;
    UCHAR PFlag[2];
    SHORT mflag;
    BOOLEAN bAllocatePoolFlag = FALSE;

    WDF_REQUEST_PARAMETERS RequestParameters;
    WDF_REQUEST_PARAMETERS_INIT(&RequestParameters);
    WdfRequestGetParameters(Request, &RequestParameters);

    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET)) {
        status = STATUS_INVALID_BUFFER_SIZE;
        RegDebug(L"HidSetReport STATUS_INVALID_BUFFER_SIZE", NULL, status);
        goto exit;
    }

    pHidPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;
    if (!pHidPacket) {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"HidSetReport STATUS_INVALID_PARAMETER", NULL, status);
        goto exit;
    }

    reportBufferLen = pHidPacket->reportBufferLen;
    if (reportBufferLen) {
        RegisterAddressFirst = pDevContext->HidSettings.CommandRegisterAddress;
        *(PUSHORT)PFlag = 0x300;
        int Type = ReportType - 2;
        if (Type) {
            if (Type != 1) {
                status = STATUS_INVALID_PARAMETER;
                RegDebug(L"HidSetReport Type STATUS_INVALID_PARAMETER", NULL, status);
                goto exit;
            }

            mflag = 0x330;
        }
        else
        {
            mflag = 0x320;
        }

        UCHAR reportId = pHidPacket->reportId;
        if (reportId >= 0xFu) {
            HeaderLength = 3;
            *(PUSHORT)PFlag = mflag | 0xF;

            PBYTE pReportDesciptorHeader = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, 3, HIDI2C_POOL_TAG);
            pReportData = pReportDesciptorHeader;
            if (!pReportDesciptorHeader) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                RegDebug(L"HidSetReport pReportDesciptorHeader STATUS_INSUFFICIENT_RESOURCES", NULL, status);
                goto exit;
            }

            bAllocatePoolFlag = TRUE;
            *(PUSHORT)pReportDesciptorHeader = 0;
            pReportDesciptorHeader[2] = 0;
            *(PUSHORT)pReportDesciptorHeader = *(PUSHORT)PFlag;
            pReportDesciptorHeader[2] = reportId;

        }
        else {
            pReportData = PFlag;
            HeaderLength = 2;
            *(PUSHORT)PFlag = mflag | reportId;
        }

        RegisterAddressSecond = pDevContext->HidSettings.DataRegisterAddress;
        pReportDesciptorData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, reportBufferLen + 2, HIDI2C_POOL_TAG);
        if (!pReportDesciptorData) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            RegDebug(L"HidSetReport pReportDesciptorData STATUS_INSUFFICIENT_RESOURCES", NULL, status);
            goto exit;
        }

        memset(pReportDesciptorData, 0, reportBufferLen + 2);
        *(PUSHORT)pReportDesciptorData = (USHORT)reportBufferLen + 2;
        memmove(pReportDesciptorData + 2, pHidPacket, reportBufferLen);

        status = SpbWriteWrite(pDevContext->SpbIoTarget, RegisterAddressFirst, pReportData, HeaderLength, RegisterAddressSecond, pReportDesciptorData, (USHORT)reportBufferLen + 2);
        if (NT_SUCCESS(status)) {
            WdfRequestSetInformation(Request, reportBufferLen);
        }
    }
    else {
        status = STATUS_BUFFER_TOO_SMALL;
        RegDebug(L"HidSetReport STATUS_BUFFER_TOO_SMALL", NULL, status);
        goto exit;
    }

exit:
    if (pReportDesciptorData)
        ExFreePoolWithTag(pReportDesciptorData, HIDI2C_POOL_TAG);
    if (pReportData && bAllocatePoolFlag)
        ExFreePoolWithTag(pReportData, HIDI2C_POOL_TAG);

    RegDebug(L"HidSetReport end", NULL, status);
    return status;

}


BOOLEAN
OnInterruptIsr(
    _In_  WDFINTERRUPT FxInterrupt,
    _In_  ULONG MessageID
)
{
    UNREFERENCED_PARAMETER(MessageID);

    NTSTATUS status = STATUS_SUCCESS;

    runtimes_OnInterruptIsr++;
    RegDebug(L"OnInterruptIsr runtimes_OnInterruptIsr",  NULL, runtimes_OnInterruptIsr);
    RegDebug(L"OnInterruptIsr runtimes_hid", NULL, runtimes_hid++);


    PUSHORT   pInputReportBuffer = NULL;
    LONG    Actual_inputReportLength = 0;
    GUID    activityId = { 0 };
    WDFREQUEST deviceResetNotificationRequest = NULL;

    WDFDEVICE FxDevice = WdfInterruptGetDevice(FxInterrupt);
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(FxDevice);

    ULONG  inputReportMaxLength = pDevContext->HidSettings.InputReportMaxLength;
    NT_ASSERT(inputReportMaxLength >= sizeof(USHORT));

    pInputReportBuffer = (PUSHORT)pDevContext->pHidInputReport;
    NT_ASSERTMSG("Input Report buffer must be allocated and non-NULL", pInputReportBuffer != NULL);

    RtlZeroMemory(pDevContext->pHidInputReport, inputReportMaxLength);

    status = SpbWritelessRead(
        pDevContext->SpbIoTarget,
        pDevContext->SpbRequest,
        (PBYTE)pInputReportBuffer,
        inputReportMaxLength
       );

    if (!NT_SUCCESS(status))
    {
        RegDebug(L"SpbWritelessRead failed inputRegister", NULL, status);
        goto exit;
    }

    USHORT Actual_HidDescriptorLength = pInputReportBuffer[0];

    // Check if this is a zero length report, which indicates a reset.// Are we expecting a Host Initated Reset?
    if (*((PUSHORT)pInputReportBuffer) == 0) {
        if (pDevContext->HostInitiatedResetActive == TRUE) {

            pDevContext->HostInitiatedResetActive = FALSE;

            if (WdfTimerStop(pDevContext->timerHandle, FALSE)) {
                WdfIoQueueStart(pDevContext->IoctlQueue);
            }

            RegDebug(L"OnInterruptIsr ok", NULL, status);
        }
        else {        
           // //
           //// This is a Device Initiated Reset. Then we need to complete
           //// the Device Reset Notification request if it exists and is 
           //// still cancelable.
           ////
           // BOOLEAN completeRequest = FALSE;

           // WdfSpinLockAcquire(pDevContext->DeviceResetNotificationSpinLock);

           // if (NULL != pDevContext->DeviceResetNotificationRequest)
           // {
           //     deviceResetNotificationRequest = pDevContext->DeviceResetNotificationRequest;
           //     pDevContext->DeviceResetNotificationRequest = NULL;

           //     status = WdfRequestUnmarkCancelable(deviceResetNotificationRequest);
           //     if (NT_SUCCESS(status))
           //     {
           //         completeRequest = TRUE;
           //     }
           //     else
           //     {
           //         NT_ASSERT(STATUS_CANCELLED == status);
           //     }
           // }

           // WdfSpinLockRelease(pDevContext->DeviceResetNotificationSpinLock);

           // if (completeRequest)
           // {
           //     status = STATUS_SUCCESS;
           //     WdfRequestComplete(deviceResetNotificationRequest, status);
           // }
           // //---------------------------------------------


            //
            status = WdfIoQueueRetrieveNextRequest(pDevContext->ResetNotificationQueue, &deviceResetNotificationRequest);
            if (!NT_SUCCESS(status)) {
                if (status == STATUS_NO_MORE_ENTRIES) {
                    RegDebug(L"OnInterruptIsr WdfIoQueueRetrieveNextRequest STATUS_NO_MORE_ENTRIES ", NULL, status);
                    goto exit;
                }
                else {
                    RegDebug(L"OnInterruptIsr WdfIoQueueRetrieveNextRequest failed ", NULL, status);
                }
            }
            else {
                WdfRequestComplete(deviceResetNotificationRequest, status);
                RegDebug(L"OnInterruptIsr  WdfRequestComplete", NULL, runtimes_OnInterruptIsr);
            }
        }

        goto exit;
    }

    //
    if (*pInputReportBuffer)
    {
 
        RegDebug(L"OnInterruptIsr pInputReportBuffer ", pInputReportBuffer, Actual_HidDescriptorLength);
        
        if (pDevContext->HostInitiatedResetActive == TRUE)
        {
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            RegDebug(L"Invalid input report returned for Reset command", NULL, status);

            goto exit;
        }

        Actual_inputReportLength = Actual_HidDescriptorLength - HID_REPORT_LENGTH_FIELD_SIZE;

        if (Actual_inputReportLength <= 0 || (ULONG)Actual_inputReportLength > inputReportMaxLength)
        {
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            RegDebug(L"Invalid input report returned inputReportActualLength", NULL, status);
            goto exit;
        }

        SetAAPThreshold(pDevContext);//当前时间表明已经用户登录了系统，可以进行获取SID操作了

        PBYTE pBuf = (PBYTE)pInputReportBuffer + 2;

        PTP_REPORT ptpReport;

        //Single finger hybrid reporting mode单指混合模式
        if (pDevContext->bHybrid_ReportingMode) {//混合报告模式状态

            //合并数据帧MergeFrame
            HYBRID_REPORT* pCurrentPartOfFrame  = &pDevContext->currentPartOfFrame;//合并帧的部分数据包

            PTP_REPORT* pCombinedPacket = &pDevContext->combinedPacket;
            RtlCopyMemory(pCurrentPartOfFrame, pBuf, sizeof(HYBRID_REPORT));

            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.ReportID", NULL, pCurrentPartOfFrame->ReportID);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.C5_BLOB", NULL, pCurrentPartOfFrame->C5_BLOB);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.Confidence", NULL, pCurrentPartOfFrame->Confidence);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.TipSwitch", NULL, pCurrentPartOfFrame->TipSwitch);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.Padding1", NULL, pCurrentPartOfFrame->Padding1);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.ContactID", NULL, pCurrentPartOfFrame->ContactID);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.X", NULL, pCurrentPartOfFrame->X);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.Y", NULL, pCurrentPartOfFrame->Y);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.ScanTime", NULL, pCurrentPartOfFrame->ScanTime);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.ContactCount", NULL, pCurrentPartOfFrame->ContactCount);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.IsButtonClicked", NULL, pCurrentPartOfFrame->IsButtonClicked);
            //RegDebug(L"OnInterruptIsr HYBRID PartOfFrame.Padding2", NULL, pCurrentPartOfFrame->Padding2);

            if (pCurrentPartOfFrame->ScanTime == pCombinedPacket->ScanTime) {//与上个数据包同属一帧
                //不要判断pCurrentFrame->Confidence && pCurrentFrame->TipSwitch是否有效，直接添加帧，否则有可能出现问题
                pDevContext->contactCountIndex++;//后续帧索引号兼计数器
                //RegDebug(L"OnInterruptIsr HYBRID_REPORT pDevContext->contactCountIndex=", NULL, pDevContext->contactCountIndex);  

                //RegDebug(L"OnInterruptIsr HYBRID_REPORT check pCurrentPartOfFrame->ContactCount=", NULL, pCurrentPartOfFrame->ContactCount);
                //RegDebug(L"OnInterruptIsr HYBRID_REPORT check PartOfFrame.TipSwitch", NULL, pCurrentPartOfFrame->TipSwitch);

                if (pDevContext->contactCountIndex == (pCombinedPacket->ContactCount - 1)) {//帧内最后一个数据包
    /*                RegDebug(L"OnInterruptIsr HYBRID_REPORT lastpacket pDevContext->contactCountIndex=", NULL, pCombinedPacket->ContactCount);
                    RegDebug(L"OnInterruptIsr HYBRID_REPORT lastpacket PartOfFrame.TipSwitch", NULL, pCurrentPartOfFrame->TipSwitch);*/
                    pDevContext->CombinedPacketReady = TRUE;//合并帧结束，可以发送
                }
            }
            else {//数据包属于新的一帧，当前合并帧结束
                RtlZeroMemory(pCombinedPacket, sizeof(PTP_REPORT));
                pDevContext->contactCountIndex = 0;//帧内索引号兼计数器重置
                pDevContext->CombinedPacketReady = FALSE;//合并帧数据状态重置
                pCombinedPacket->ContactCount = pCurrentPartOfFrame->ContactCount;//首帧包含接触点数量
                //RegDebug(L"OnInterruptIsr HYBRID_REPORT pCombinedPacket->ContactCount==", NULL, pCombinedPacket->ContactCount);
                pCombinedPacket->IsButtonClicked = pCurrentPartOfFrame->IsButtonClicked;
                //RegDebug(L"OnInterruptIsr HYBRID_REPORT pCombinedPacket->IsButtonClicked==", NULL, pCombinedPacket->IsButtonClicked);
                pCombinedPacket->ReportID = pCurrentPartOfFrame->ReportID;
                //pCombinedPacket->ReportID = FAKE_REPORTID_MULTITOUCH;
                pCombinedPacket->ScanTime = pCurrentPartOfFrame->ScanTime;

                if (pCombinedPacket->ContactCount == 1) {//合并帧只有一个数据包时，立即发送而不需要等待下一个分数据包
                    //RegDebug(L"OnInterruptIsr HYBRID_REPORT pCombinedPacket->ContactCount only1", NULL, pCombinedPacket->ContactCount);
                    pDevContext->CombinedPacketReady = TRUE;//合并帧结束，可以发送了
                }
            }

            //添加当前帧到合并数据包中，不要判断pCurrentFrame->Confidence && pCurrentFrame->TipSwitch是否有效，直接添加帧，否则有可能出现问题
            BYTE i = pDevContext->contactCountIndex;
            pCombinedPacket->Contacts[i].Confidence = pCurrentPartOfFrame->Confidence;
            pCombinedPacket->Contacts[i].ContactID = pCurrentPartOfFrame->ContactID;
            pCombinedPacket->Contacts[i].TipSwitch = pCurrentPartOfFrame->TipSwitch;
            pCombinedPacket->Contacts[i].Padding = 0;
            pCombinedPacket->Contacts[i].X = pCurrentPartOfFrame->X;
            pCombinedPacket->Contacts[i].Y = pCurrentPartOfFrame->Y;


            if (pDevContext->CombinedPacketReady) {//合并帧准备好了,发送合并帧
                RtlCopyMemory(&ptpReport, pCombinedPacket, sizeof(PTP_REPORT));
                ptpReport.ReportID = FAKE_REPORTID_MULTITOUCH;
                //RegDebug(L"OnInterruptIsr HYBRID ptpReport=", &ptpReport, sizeof(PTP_REPORT));

                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.ReportID", NULL, ptpReport.ReportID);
                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.IsButtonClicked", NULL, ptpReport.IsButtonClicked);
                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.ScanTime", NULL, ptpReport.ScanTime);
                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.ContactCount", NULL, ptpReport.ContactCount);

                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT..Contacts[0].Confidence ", NULL, ptpReport.Contacts[0].Confidence);
                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.Contacts[0].ContactID ", NULL, ptpReport.Contacts[0].ContactID);
                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.Contacts[0].TipSwitch ", NULL, ptpReport.Contacts[0].TipSwitch);
                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.Contacts[0].Padding ", NULL, ptpReport.Contacts[0].Padding);
                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.Contacts[0].X ", NULL, ptpReport.Contacts[0].X);
                //RegDebug(L"OnInterruptIsr HYBRID PTP_REPORT.Contacts[0].Y ", NULL, ptpReport.Contacts[0].Y);

                if (ptpReport.ScanTime > 0x64) {
                    ptpReport.ScanTime = 0x64;
                }
                goto parse;

            }
   
            goto exit;//等待下一个帧
        }


        //Parallel mode

        //
        if (!pDevContext->PtpInputModeOn) {//输入集合异常模式下  
            ////发送原始报告
            //status = SendOriginalReport(pDevContext, pBuf, Actual_inputReportLength);
            //if (!NT_SUCCESS(status)) {
            //    RegDebug(L"OnInterruptIsr SendOriginalReport failed", NULL, runtimes_ioControl);
            //}
            RegDebug(L"OnInterruptIsr PtpInputModeOn not ready", NULL, runtimes_ioControl);
            goto exit;
        }


        ptpReport = *(PPTP_REPORT)pBuf;
        ptpReport.ReportID = FAKE_REPORTID_MULTITOUCH;
        //RegDebug(L"OnInterruptIsr PTP_REPORT.ReportID", NULL, ptpReport.ReportID);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.IsButtonClicked", NULL, ptpReport.IsButtonClicked);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.ScanTime", NULL, ptpReport.ScanTime);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.ContactCount", NULL, ptpReport.ContactCount);

        //RegDebug(L"OnInterruptIsr PTP_REPORT..Contacts[0].Confidence ", NULL, ptpReport.Contacts[0].Confidence);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].ContactID ", NULL, ptpReport.Contacts[0].ContactID);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].TipSwitch ", NULL, ptpReport.Contacts[0].TipSwitch);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].Padding ", NULL, ptpReport.Contacts[0].Padding);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].X ", NULL, ptpReport.Contacts[0].X);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].Y ", NULL, ptpReport.Contacts[0].Y);



parse:
        if (!pDevContext->bMouseLikeTouchPad_Mode) {//原版触控板操作方式直接发送原始报告
            PTP_PARSER* tps = &pDevContext->tp_settings;
            if (ptpReport.IsButtonClicked) {
                //需要进行离开判定，否则本次或下次进入MouseLikeTouchPad解析器后关系bPhysicalButtonUp还会被内部的代码段执行造成未知问题
                tps->bPhysicalButtonUp = FALSE;
                RegDebug(L"OnInterruptIsr bPhysicalButtonUp FALSE", NULL, FALSE);
            }
            else {
                if (!tps->bPhysicalButtonUp) {
                    tps->bPhysicalButtonUp = TRUE;
                    RegDebug(L"OnInterruptIsr bPhysicalButtonUp TRUE", NULL, TRUE);

                    if (ptpReport.ContactCount == 4 && !pDevContext->bMouseLikeTouchPad_Mode) {//四指按压触控板物理按键时，切换回仿鼠标式触摸板模式，
                        pDevContext->bMouseLikeTouchPad_Mode = TRUE;
                        RegDebug(L"OnInterruptIsr bMouseLikeTouchPad_Mode TRUE", NULL, status);

                        //切换回仿鼠标式触摸板模式的同时也恢复滚轮功能和实现方式
                        pDevContext->bWheelDisabled = FALSE;
                        RegDebug(L"OnInterruptIsr bWheelDisabled=", NULL, pDevContext->bWheelDisabled);
                        pDevContext->bWheelScrollMode = FALSE;
                        RegDebug(L"OnInterruptIsr bWheelScrollMode=", NULL, pDevContext->bWheelScrollMode);
                    }
                }
            }

            //windows原版的PTP精确式触摸板操作方式，直接发送PTP报告
            status = SendPtpMultiTouchReport(pDevContext, &ptpReport, sizeof(PTP_REPORT));
            if (!NT_SUCCESS(status)) {
                RegDebug(L"OnInterruptIsr SendPtpMultiTouchReport ptpReport failed", NULL, status);
            }
           
        }
        else {
            //MouseLikeTouchPad解析器
            MouseLikeTouchPad_parse(pDevContext, &ptpReport);
        }

        RegDebug(L"OnInterruptIsr SendReport end", NULL, runtimes_ioControl);
        goto exit;

    }

    
    

   

exit:
    //RegDebug(L"OnInterruptIsr end", NULL, status);
    return TRUE;
}


VOID
PowerIdleIrpWorkitem(
    _In_  WDFWORKITEM IdleWorkItem
)
{

    NTSTATUS status;

    PWORKITEM_CONTEXT pWorkItemContext = GetWorkItemContext(IdleWorkItem);
    NT_ASSERT(pWorkItemContext != NULL);

    PDEVICE_CONTEXT pDevContext = GetDeviceContext(pWorkItemContext->FxDevice);
    NT_ASSERT(pDevContext != NULL);

    PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO idleCallbackInfo = _HidGetIdleCallbackInfo(pWorkItemContext->FxRequest);//??效果等同与下列注释的4行代码

    idleCallbackInfo->IdleCallback(idleCallbackInfo->IdleContext);//

    //{
    //    PIRP pIrp = WdfRequestWdmGetIrp(pWorkItemContext->FxRequest);//
    //    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(pIrp);//即是PIO_STACK_LOCATION IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    //    PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO idleCallbackInfo = (PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    //    idleCallbackInfo->IdleCallback(idleCallbackInfo->IdleContext);
    //}

    //
    status = WdfRequestForwardToIoQueue(
        pWorkItemContext->FxRequest,
        pDevContext->IdleQueue);

    if (!NT_SUCCESS(status))
    {
        //
        NT_ASSERTMSG("WdfRequestForwardToIoQueue to IdleQueue failed!", FALSE);
        RegDebug(L"PowerIdleIrpWorkitem WdfRequestForwardToIoQueue IdleQueue failed", NULL, status);

        WdfRequestComplete(pWorkItemContext->FxRequest, status);
    }
    else
    {
        RegDebug(L"Forwarded idle notification Request to IdleQueue", NULL, status);
    }


    WdfObjectDelete(IdleWorkItem);

    RegDebug(L"PowerIdleIrpWorkitem end", NULL, status);
    return;
}


VOID
PowerIdleIrpCompletion(
    _In_ PDEVICE_CONTEXT    FxDeviceContext
)
{
    NTSTATUS status = STATUS_SUCCESS;

    {
        WDFREQUEST request = NULL;
        status = WdfIoQueueRetrieveNextRequest(
            FxDeviceContext->IdleQueue,
            &request);

        if (!NT_SUCCESS(status) || (request == NULL))
        {
            RegDebug(L"WdfIoQueueRetrieveNextRequest failed to find idle notification request in IdleQueue", NULL, status);//STATUS_NO_MORE_ENTRIES
        }
        else
        {
            WdfRequestComplete(request, status);
            RegDebug(L"Completed idle notification Request from IdleQueue", NULL, status);
        }
    }

    RegDebug(L"PowerIdleIrpCompletion end", NULL, status);//
    return;
}



NTSTATUS
PtpReportFeatures(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
)
{
    NTSTATUS Status;
    PDEVICE_CONTEXT pDevContext;
    PHID_XFER_PACKET pHidPacket;
    WDF_REQUEST_PARAMETERS RequestParameters;
    size_t ReportSize;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    pDevContext = GetDeviceContext(Device);

    WDF_REQUEST_PARAMETERS_INIT(&RequestParameters);
    WdfRequestGetParameters(Request, &RequestParameters);

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_XFER_PACKET))
    {
        RegDebug(L"STATUS_BUFFER_TOO_SMALL", NULL, 0x12345678);
        Status = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    pHidPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;
    if (pHidPacket == NULL)
    {
        RegDebug(L"STATUS_INVALID_DEVICE_REQUEST", NULL, 0x12345678);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    UCHAR reportId = pHidPacket->reportId;
    if(reportId== FAKE_REPORTID_DEVICE_CAPS){//FAKE_REPORTID_DEVICE_CAPS//pDevContext->REPORTID_DEVICE_CAPS
            ReportSize = sizeof(PTP_DEVICE_CAPS_FEATURE_REPORT);
            if (pHidPacket->reportBufferLen < ReportSize) {
                Status = STATUS_INVALID_BUFFER_SIZE;
                RegDebug(L"PtpGetFeatures REPORTID_DEVICE_CAPS STATUS_INVALID_BUFFER_SIZE", NULL, pHidPacket->reportId);
                goto exit;
            }

            PPTP_DEVICE_CAPS_FEATURE_REPORT capsReport = (PPTP_DEVICE_CAPS_FEATURE_REPORT)pHidPacket->reportBuffer;

            capsReport->MaximumContactPoints = PTP_MAX_CONTACT_POINTS;// pDevContext->CONTACT_COUNT_MAXIMUM;// PTP_MAX_CONTACT_POINTS;
            capsReport->ButtonType = PTP_BUTTON_TYPE_CLICK_PAD;// pDevContext->PAD_TYPE;// PTP_BUTTON_TYPE_CLICK_PAD;
            capsReport->ReportID = FAKE_REPORTID_DEVICE_CAPS;// pDevContext->REPORTID_DEVICE_CAPS;//FAKE_REPORTID_DEVICE_CAPS
            RegDebug(L"PtpGetFeatures pHidPacket->reportId REPORTID_DEVICE_CAPS", NULL, pHidPacket->reportId);
            RegDebug(L"PtpGetFeatures REPORTID_DEVICE_CAPS MaximumContactPoints", NULL, capsReport->MaximumContactPoints);
            RegDebug(L"PtpGetFeatures REPORTID_DEVICE_CAPS REPORTID_DEVICE_CAPS ButtonType", NULL, capsReport->ButtonType);
    }
    else if (reportId == FAKE_REPORTID_PTPHQA) {//FAKE_REPORTID_PTPHQA//pDevContext->REPORTID_PTPHQA
            // Size sanity check
            ReportSize = sizeof(PTP_DEVICE_HQA_CERTIFICATION_REPORT);
            if (pHidPacket->reportBufferLen < ReportSize)
            {
                Status = STATUS_INVALID_BUFFER_SIZE;
                RegDebug(L"PtpGetFeatures REPORTID_PTPHQA STATUS_INVALID_BUFFER_SIZE", NULL, pHidPacket->reportId);
                goto exit;
            }

            PPTP_DEVICE_HQA_CERTIFICATION_REPORT certReport = (PPTP_DEVICE_HQA_CERTIFICATION_REPORT)pHidPacket->reportBuffer;

            *certReport->CertificationBlob = DEFAULT_PTP_HQA_BLOB;
            certReport->ReportID = FAKE_REPORTID_PTPHQA;//FAKE_REPORTID_PTPHQA//pDevContext->REPORTID_PTPHQA
            pDevContext->PtpInputModeOn = TRUE;//测试

            RegDebug(L"PtpGetFeatures pHidPacket->reportId REPORTID_PTPHQA", NULL, pHidPacket->reportId);

    }
    else{

            Status = STATUS_NOT_SUPPORTED;
            RegDebug(L"PtpGetFeatures pHidPacket->reportId STATUS_NOT_SUPPORTED", NULL, pHidPacket->reportId);
            goto exit;
    }
    
    WdfRequestSetInformation(Request, ReportSize);
    RegDebug(L"PtpGetFeatures STATUS_SUCCESS pDeviceContext->PtpInputOn", NULL, pDevContext->PtpInputModeOn);


exit:

    return Status;
}



NTSTATUS
HidGetFeature(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request,
    HID_REPORT_TYPE ReportType
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PHID_XFER_PACKET pHidPacket;
    size_t reportBufferLen;
    USHORT HeaderLength;
    USHORT RegisterAddressFirst;
    USHORT RegisterAddressSecond;
    PBYTE pReportData = NULL;
    PBYTE pReportDesciptorData = NULL;
    SHORT PFlag;
    SHORT mflag;
    BOOLEAN bAllocatePoolFlag = FALSE;


    WDF_REQUEST_PARAMETERS RequestParameters;
    WDF_REQUEST_PARAMETERS_INIT(&RequestParameters);
    WdfRequestGetParameters(Request, &RequestParameters);

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength >= sizeof(HID_XFER_PACKET)) {
        RegDebug(L"HidGetFeature OutputBufferLength=", NULL, (ULONG)RequestParameters.Parameters.DeviceIoControl.OutputBufferLength);
        RegDebug(L"HidGetFeature Parameters.Write.Length=", NULL, (ULONG)RequestParameters.Parameters.Write.Length);
        pHidPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;
        if (!pHidPacket) {
            status = STATUS_INVALID_PARAMETER;
            RegDebug(L"HidGetFeature STATUS_INVALID_PARAMETER", NULL, status);
            goto exit;
        }
        RegDebug(L"HidGetFeature pHidPacket=", pHidPacket, sizeof(PHID_XFER_PACKET));
        RegDebug(L"HidGetFeature pHidPacket->reportBufferLen=", NULL, pHidPacket->reportBufferLen);
        RegDebug(L"HidGetFeature pHidPacket->reportId=", NULL, pHidPacket->reportId);

        reportBufferLen = pHidPacket->reportBufferLen;
        if (reportBufferLen) {
            RegisterAddressFirst = pDevContext->HidSettings.CommandRegisterAddress;
            PFlag = 0x200;
            int Type = ReportType - 1;
            if (Type) {
                if (Type != 2) {
                    status = STATUS_INVALID_PARAMETER;
                    RegDebug(L"HidGetFeature Type STATUS_INVALID_PARAMETER", NULL, status);
                    goto exit;
                }

                mflag = 0x230;
            }
            else
            {
                mflag = 0x210;
            }

            UCHAR reportId = pHidPacket->reportId;
            if (reportId == pDevContext->REPORTID_DEVICE_CAPS) {
                RegDebug(L"HidGetFeature REPORTID_DEVICE_CAPS reportId=", NULL, reportId);
            }
            else if (reportId == pDevContext->REPORTID_PTPHQA) {
                RegDebug(L"HidGetFeature REPORTID_PTPHQA reportId=", NULL, reportId);
            }
            else {
                RegDebug(L"HidGetFeature Not Support reportId=", NULL, reportId);
            }
            if (reportId >= 0xFu) {
                HeaderLength = 3;
                PFlag = mflag | 0xF;

                PBYTE pReportDesciptorHeader = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, 3, HIDI2C_POOL_TAG);
                pReportData = pReportDesciptorHeader;
                if (!pReportDesciptorHeader) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    RegDebug(L"HidGetFeature pReportDesciptorHeader STATUS_INSUFFICIENT_RESOURCES", NULL, status);
                    goto exit;
                }

                bAllocatePoolFlag = TRUE;
                *(PUSHORT)pReportDesciptorHeader = 0;
                pReportDesciptorHeader[2] = 0;
                *(PUSHORT)pReportDesciptorHeader = PFlag;
                pReportDesciptorHeader[2] = reportId;

            }
            else {
                pReportData = (PUCHAR)&PFlag;
                HeaderLength = 2;
                PFlag = mflag | reportId;
            }
            RegDebug(L"HidGetFeature pReportDataHeader=", pReportData, HeaderLength);

            RegisterAddressSecond = pDevContext->HidSettings.DataRegisterAddress;
            pReportDesciptorData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, reportBufferLen + 2, HIDI2C_POOL_TAG);
            if (!pReportDesciptorData) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                RegDebug(L"HidGetFeature pReportDesciptorData STATUS_INSUFFICIENT_RESOURCES", NULL, status);
                goto exit;
            }

            memset(pReportDesciptorData, 0, reportBufferLen + 2);

            ULONG DelayUs = 0;
            status = SpbWriteRead(pDevContext->SpbIoTarget, RegisterAddressFirst, pReportData, HeaderLength, RegisterAddressSecond, pReportDesciptorData, (USHORT)reportBufferLen + 2, DelayUs);
            if (NT_SUCCESS(status)) {
                size_t ReportSize = *(PUSHORT)pReportDesciptorData - 2;
                if (!ReportSize) {
                    RegDebug(L"HidGetFeature ReportSize err", NULL, (ULONG)ReportSize);
                }
                else {
                    if (reportBufferLen < ReportSize) {
                        status = STATUS_BUFFER_TOO_SMALL;
                        RegDebug(L"HidGetFeature ReportSize STATUS_BUFFER_TOO_SMALL", NULL, status);
                        goto exit;

                    }

                    memmove(pHidPacket, pReportDesciptorData + 2, ReportSize);
                    RegDebug(L"HidGetFeature pReportDesciptorData=", pReportDesciptorData, (ULONG)reportBufferLen + 2);
                    if (reportId == pDevContext->REPORTID_DEVICE_CAPS) {
                        RegDebug(L"HidGetFeature REPORTID_DEVICE_CAPS pReportDesciptorData=", pReportDesciptorData, (ULONG)ReportSize + 2);
                    }
                    else if (reportId == pDevContext->REPORTID_PTPHQA) {
                        RegDebug(L"HidGetFeature REPORTID_PTPHQA pReportDesciptorData=", pReportDesciptorData, (ULONG)ReportSize + 2);
                    }
                    else {
                        RegDebug(L"HidGetFeature Not Support pReportDesciptorData=", pReportDesciptorData, (ULONG)ReportSize + 2);
                    }
                    WdfRequestSetInformation(Request, ReportSize);

                }
            }
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
            RegDebug(L"HidGetFeature STATUS_BUFFER_TOO_SMALL", NULL, status);
            goto exit;
        }

        if (NT_SUCCESS(status)) {
            goto exit;
        }
    }

    status = STATUS_INVALID_BUFFER_SIZE;
    RegDebug(L"HidGetFeature STATUS_INVALID_BUFFER_SIZE", NULL, status);

exit:
    if (pReportDesciptorData)
        ExFreePoolWithTag(pReportDesciptorData, HIDI2C_POOL_TAG);
    if (pReportData && bAllocatePoolFlag)
        ExFreePoolWithTag(pReportData, HIDI2C_POOL_TAG);

    RegDebug(L"HidGetFeature end", NULL, status);
    return status;

}


NTSTATUS
HidSetFeature(
    PDEVICE_CONTEXT pDevContext,
    WDFREQUEST Request,
    HID_REPORT_TYPE ReportType
)
{
    UNREFERENCED_PARAMETER(ReportType);
    NTSTATUS status = STATUS_SUCCESS;

    USHORT reportLength;
    USHORT HeaderLength;
    USHORT RegisterAddressFirst;
    USHORT RegisterAddressSecond;
    PBYTE pReportHeaderData = NULL;
    PBYTE pFeatureReportData = NULL;
    UCHAR HeaderData2[2];
    UCHAR HeaderData3[3];
    UCHAR reportID = 0;
    UCHAR reportData = 0;
    UCHAR reportDataSize = 0;

    PHID_XFER_PACKET pHidPacket;

    WDF_REQUEST_PARAMETERS RequestParameters;
    WDF_REQUEST_PARAMETERS_INIT(&RequestParameters);
    WdfRequestGetParameters(Request, &RequestParameters);

    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET)) {
        status = STATUS_INVALID_BUFFER_SIZE;
        RegDebug(L"HidSetFeature STATUS_INVALID_BUFFER_SIZE", NULL, status);
        goto exit;
    }

    pHidPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;
    if (!pHidPacket) {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"HidSetFeature STATUS_INVALID_PARAMETER", NULL, status);
        goto exit;
    }

    ULONG reportBufferLen = pHidPacket->reportBufferLen;
    if (!reportBufferLen) {
        status = STATUS_BUFFER_TOO_SMALL;
        RegDebug(L"HidSetFeature STATUS_BUFFER_TOO_SMALL", NULL, status);
        goto exit;
    }

    UCHAR reportId = pHidPacket->reportId;
    if (reportId == FAKE_REPORTID_INPUTMODE) {//FAKE_REPORTID_INPUTMODE
        reportID = pDevContext->REPORTID_INPUT_MODE;//替换为真实值
        reportDataSize = pDevContext->REPORTSIZE_INPUT_MODE;
        reportData = PTP_COLLECTION_WINDOWS;
        //RegDebug(L"HidSetFeature PTP_COLLECTION_WINDOWS reportDataSize=", NULL, reportDataSize);
    }
    else if (reportId == FAKE_REPORTID_FUNCTION_SWITCH) {//FAKE_REPORTID_FUNCTION_SWITCH
        reportID = pDevContext->REPORTID_FUNCTION_SWITCH;//替换为真实值
        reportDataSize = pDevContext->REPORTSIZE_FUNCTION_SWITCH;
        reportData = PTP_SELECTIVE_REPORT_Button_Surface_ON;
        //RegDebug(L"HidSetFeature PTP_SELECTIVE_REPORT_Button_Surface_ON reportDataSize=", NULL, reportDataSize);
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"HidSetFeature reportId err", NULL, status);
        goto exit;
    }


    if (reportID >= 0xFu) {
        HeaderLength = 3;
        pReportHeaderData = HeaderData3;
        *(PUSHORT)pReportHeaderData = 0x033F;//0x0330 | 0xF
        pReportHeaderData[2] = reportID;
        RegDebug(L"HidSetFeature reportID>=0xF pReportHeaderData=", pReportHeaderData, HeaderLength);
    }
    else {
        HeaderLength = 2;
        pReportHeaderData = HeaderData2;
        *(PUSHORT)pReportHeaderData = 0x0330 | reportID;   //USHORT左边字节为低位LowByte右边字节为高位HighByte,
        RegDebug(L"HidSetFeature reportID<0xF pReportHeaderData=", pReportHeaderData, HeaderLength);
    }

    reportLength = 3 + reportDataSize;
    pFeatureReportData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, reportLength, HIDI2C_POOL_TAG);
    if (!pFeatureReportData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"HidSetFeature pFeatureReportData STATUS_INSUFFICIENT_RESOURCES", NULL, status);
        goto exit;
    }

    RtlZeroMemory(pFeatureReportData, reportLength);
    *(PUSHORT)pFeatureReportData = reportLength;
    pFeatureReportData[2] = reportID;//REPORTID_REPORTMODE或者REPORTID_FUNCTION_SWITCH
    pFeatureReportData[3] = reportData;//PTP_COLLECTION_WINDOWS或者PTP_SELECTIVE_REPORT_Button_Surface_ON

    if (reportID == pDevContext->REPORTID_INPUT_MODE) {//SetType== PTP_FEATURE_INPUT_COLLECTION
        RegDebug(L"HidSetFeature PTP_FEATURE_INPUT_COLLECTION pFeatureReportData=", pFeatureReportData, reportLength);
    }
    else if (reportID == pDevContext->REPORTID_FUNCTION_SWITCH){//SetType== PTP_FEATURE_SELECTIVE_REPORTING
        RegDebug(L"HidSetFeature PTP_FEATURE_SELECTIVE_REPORTING pFeatureReportData=", pFeatureReportData, reportLength);
    }

    RegisterAddressFirst = pDevContext->HidSettings.CommandRegisterAddress;
    RegisterAddressSecond = pDevContext->HidSettings.DataRegisterAddress;

    status = SpbWriteWrite(pDevContext->SpbIoTarget, RegisterAddressFirst, pReportHeaderData, HeaderLength, RegisterAddressSecond, pFeatureReportData, reportLength);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"HidSetFeature SpbWriteWrite err", NULL, status);
    }
    WdfRequestSetInformation(Request, reportDataSize);
    if (reportID == pDevContext->REPORTID_INPUT_MODE) {//SetType== PTP_FEATURE_INPUT_COLLECTION
        pDevContext->SetInputModeOK = TRUE;
    }
    else if (reportID == pDevContext->REPORTID_FUNCTION_SWITCH) {//SetType== PTP_FEATURE_SELECTIVE_REPORTING
        pDevContext->SetFunSwicthOK = TRUE;
    }
    
    if (pDevContext->SetInputModeOK && pDevContext->SetFunSwicthOK) {
        pDevContext->PtpInputModeOn = TRUE;
        RegDebug(L"HidSetFeature PtpInputModeOn=", NULL, pDevContext->PtpInputModeOn);
    }

exit:
    if (pFeatureReportData) {
        ExFreePoolWithTag(pFeatureReportData, HIDI2C_POOL_TAG);
    }

    RegDebug(L"HidSetFeature end", NULL, status);
    return status;
}

NTSTATUS
PtpSetFeature(
    PDEVICE_CONTEXT pDevContext,
    BOOLEAN SetType
)
{
    NTSTATUS status = STATUS_SUCCESS;

    USHORT reportLength;
    USHORT HeaderLength;
    USHORT RegisterAddressFirst;
    USHORT RegisterAddressSecond;
    PBYTE pReportHeaderData = NULL;
    PBYTE pFeatureReportData = NULL;
    UCHAR HeaderData2[2];
    UCHAR HeaderData3[3];
    UCHAR reportID = 0;
    UCHAR reportData = 0;
    UCHAR reportDataSize = 0;


    if (SetType== PTP_FEATURE_INPUT_COLLECTION) {
        reportID = pDevContext->REPORTID_INPUT_MODE;////reportID//yoga14s为0x04,matebook为0x03
        reportDataSize = pDevContext->REPORTSIZE_INPUT_MODE;
        reportData = PTP_COLLECTION_WINDOWS;
        RegDebug(L"PtpSetFeature PTP_COLLECTION_WINDOWS reportDataSize=", NULL, reportDataSize);
    }
    else {//SetType== PTP_FEATURE_SELECTIVE_REPORTING
        reportID = pDevContext->REPORTID_FUNCTION_SWITCH;////reportID//yoga14s为0x06,matebook为0x05
        reportDataSize = pDevContext->REPORTSIZE_FUNCTION_SWITCH;
        reportData = PTP_SELECTIVE_REPORT_Button_Surface_ON;
        RegDebug(L"PtpSetFeature PTP_SELECTIVE_REPORT_Button_Surface_ON reportDataSize=", NULL, reportDataSize);
    }
    
    if (reportID >= 0xFu) {
        HeaderLength = 3;
        pReportHeaderData = HeaderData3;
        *(PUSHORT)pReportHeaderData = 0x033F;//0x0330 | 0xF
        pReportHeaderData[2] = reportID;
        RegDebug(L"PtpSetFeature reportID>=0xF pReportHeaderData=", pReportHeaderData, HeaderLength);
    }
    else {
        HeaderLength = 2;
        pReportHeaderData = HeaderData2;
        *(PUSHORT)pReportHeaderData = 0x0330 | reportID;   //USHORT左边字节为低位LowByte右边字节为高位HighByte,
        RegDebug(L"PtpSetFeature reportID<0xF pReportHeaderData=", pReportHeaderData, HeaderLength);
    }


    //USHORT hxp_size = sizeof(HID_XFER_PACKET);
    //RegDebug(L"PtpSetFeature hxp_size=", NULL, hxp_size);

    //reportLength = (USHORT)(hxp_size + reportDataSize + 2);
    //RegDebug(L"PtpSetFeature reportLength=", NULL, reportLength);

    //pFeatureReportData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, reportLength, HIDI2C_POOL_TAG);
    //if (!pFeatureReportData) {
    //    status = STATUS_INSUFFICIENT_RESOURCES;
    //    RegDebug(L"PtpSetFeature pFeatureReportData STATUS_INSUFFICIENT_RESOURCES", NULL, status);
    //    goto exit;
    //}
    //
    //static char buffer[32];
    //RtlZeroMemory(buffer, 32);
    //HID_XFER_PACKET* hxp = (HID_XFER_PACKET*)buffer;
    //hxp->reportBuffer = (PUCHAR)hxp + hxp_size;
    //hxp->reportBufferLen = reportDataSize;
    //hxp->reportId = reportID;
    //hxp->reportBuffer[0] = reportData; // 
    //RegDebug(L"PtpSetFeature hxp->reportId=", NULL, hxp->reportId);

    //RtlZeroMemory(pFeatureReportData, reportLength);
    //*(PUSHORT)pFeatureReportData = reportLength;
    //RtlCopyMemory(pFeatureReportData + 2, hxp, reportLength - 2);
    //RegDebug(L"PtpSetFeature pFeatureReportData=", pFeatureReportData, reportLength);



    reportLength = 3 + reportDataSize;
    pFeatureReportData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, reportLength, HIDI2C_POOL_TAG);
    if (!pFeatureReportData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"PtpSetFeature pFeatureReportData STATUS_INSUFFICIENT_RESOURCES", NULL, status);
        goto exit;
    }

    RtlZeroMemory(pFeatureReportData, reportLength);
    *(PUSHORT)pFeatureReportData = reportLength;
    pFeatureReportData[2] = reportID;//REPORTID_REPORTMODE或者REPORTID_FUNCTION_SWITCH
    pFeatureReportData[3] = reportData;//PTP_COLLECTION_WINDOWS或者PTP_SELECTIVE_REPORT_Button_Surface_ON

    if (SetType == PTP_FEATURE_INPUT_COLLECTION) {
        RegDebug(L"PtpSetFeature PTP_FEATURE_INPUT_COLLECTION pFeatureReportData=", pFeatureReportData, reportLength);
    }
    else {//SetType== PTP_FEATURE_SELECTIVE_REPORTING
        RegDebug(L"PtpSetFeature PTP_FEATURE_SELECTIVE_REPORTING pFeatureReportData=", pFeatureReportData, reportLength);
    }

    RegisterAddressFirst = pDevContext->HidSettings.CommandRegisterAddress;
    RegisterAddressSecond = pDevContext->HidSettings.DataRegisterAddress;

    status = SpbWriteWrite(pDevContext->SpbIoTarget, RegisterAddressFirst, pReportHeaderData, HeaderLength, RegisterAddressSecond, pFeatureReportData, reportLength);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"PtpSetFeature SpbWriteWrite err", NULL, status);
    }

exit:
    if (pFeatureReportData) {
        ExFreePoolWithTag(pFeatureReportData, HIDI2C_POOL_TAG);
    }

    RegDebug(L"PtpSetFeature end", NULL, status);
    return status;

}


NTSTATUS
GetReportDescriptor(
    PDEVICE_CONTEXT pDevContext
)
{
    RegDebug(L"GetReportDescriptor start", NULL, runtimes_ioControl);

    NTSTATUS status = STATUS_SUCCESS;

    USHORT RegisterAddress = pDevContext->HidSettings.ReportDescriptorAddress;
    USHORT ReportLength = pDevContext->HidSettings.ReportDescriptorLength;

    PBYTE pReportDesciptorData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, ReportLength, HIDI2C_POOL_TAG);
    if (!pReportDesciptorData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"GetReportDescriptor ExAllocatePoolWithTag failed", NULL, status);
        return status;
    }

    ULONG DelayUs = 0;
    status = SpbRead(pDevContext->SpbIoTarget, RegisterAddress, pReportDesciptorData, ReportLength, DelayUs, HIDI2C_REQUEST_DEFAULT_TIMEOUT);///原先单位为秒太大改为ms
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"GetReportDescriptor SpbRead failed", NULL, status);
        return status;
    }

    pDevContext->pReportDesciptorData = pReportDesciptorData;
    RegDebug(L"GetReportDescriptor pReportDesciptorData=", pReportDesciptorData, ReportLength);

    return status;
}


NTSTATUS
AnalyzeHidReportDescriptor(
    PDEVICE_CONTEXT pDevContext
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE descriptor = pDevContext->pReportDesciptorData;
    if (!descriptor) {
        RegDebug(L"AnalyzeHidReportDescriptor pReportDesciptorData err", NULL, status);
        return STATUS_UNSUCCESSFUL;
    }

    USHORT descriptorLen = pDevContext->HidSettings.ReportDescriptorLength;
    PTP_PARSER* tp = &pDevContext->tp_settings;

    int depth = 0;
    BYTE usagePage = 0;
    BYTE reportId = 0;
    BYTE reportSize = 0;
    USHORT reportCount = 0;
    BYTE lastUsage = 0;
    BYTE lastCollection = 0;//改变量能够用于准确判定PTP、MOUSE集合输入报告的reportID
    bool inConfigTlc = false;
    bool inTouchTlc = false;
    bool inMouseTlc = false;
    USHORT logicalMax = 0;
    USHORT physicalMax = 0;
    UCHAR unitExp = 0;
    UCHAR unit = 0;

    for (size_t i = 0; i < descriptorLen;) {
        BYTE type = descriptor[i++];
        int size = type & 3;
        if (size == 3) {
            size++;
        }
        BYTE* value = &descriptor[i];
        i += size;

        if (type == HID_TYPE_BEGIN_COLLECTION) {
            depth++;
            if (depth == 1 && usagePage == HID_USAGE_PAGE_DIGITIZER && lastUsage == HID_USAGE_CONFIGURATION) {
                inConfigTlc = true;
                lastCollection = HID_USAGE_CONFIGURATION;
                //RegDebug(L"AnalyzeHidReportDescriptor inConfigTlc", NULL, 0);
            }
            else if (depth == 1 && usagePage == HID_USAGE_PAGE_DIGITIZER && lastUsage == HID_USAGE_DIGITIZER_TOUCH_PAD) {
                inTouchTlc = true;
                lastCollection = HID_USAGE_DIGITIZER_TOUCH_PAD;
                //RegDebug(L"AnalyzeHidReportDescriptor inTouchTlc", NULL, 0);
            }
            else if (depth == 1 && usagePage == HID_USAGE_PAGE_GENERIC && lastUsage == HID_USAGE_GENERIC_MOUSE) {
                inMouseTlc = true;
                lastCollection = HID_USAGE_GENERIC_MOUSE;
                //RegDebug(L"AnalyzeHidReportDescriptor inMouseTlc", NULL, 0);
            }
        }
        else if (type == HID_TYPE_END_COLLECTION) {
            depth--;

            //下面3个Tlc状态更新是有必要的，可以防止后续相关集合Tlc错误判定发生
            if (depth == 0 && inConfigTlc) {
                inConfigTlc = false;
                //RegDebug(L"AnalyzeHidReportDescriptor inConfigTlc end", NULL, 0);
            }
            else if (depth == 0 && inTouchTlc) {
                inTouchTlc = false;
                //RegDebug(L"AnalyzeHidReportDescriptor inTouchTlc end", NULL, 0);
            }
            else if (depth == 0 && inMouseTlc) {
                inMouseTlc = false;
                //RegDebug(L"AnalyzeHidReportDescriptor inMouseTlc end", NULL, 0);
            }

        }
        else if (type == HID_TYPE_USAGE_PAGE) {
            usagePage = *value;
        }
        else if (type == HID_TYPE_USAGE) {
            lastUsage = *value; 
        }
        else if (type == HID_TYPE_REPORT_ID) {
            reportId = *value;
        }
        else if (type == HID_TYPE_REPORT_SIZE) {
            reportSize = *value;
        }
        else if (type == HID_TYPE_REPORT_COUNT) {
            reportCount = *value;
        }
        else if (type == HID_TYPE_REPORT_COUNT_2) {
            reportCount = *(PUSHORT)value;
        }
        else if (type == HID_TYPE_LOGICAL_MINIMUM) {
            logicalMax = *value;
        }
        else if (type == HID_TYPE_LOGICAL_MAXIMUM_2) {
            logicalMax = *(PUSHORT)value;
        }
        else if (type == HID_TYPE_PHYSICAL_MAXIMUM) {
            physicalMax = *value;
        }
        else if (type == HID_TYPE_PHYSICAL_MAXIMUM_2) {
            physicalMax= *(PUSHORT)value;
        }
        else if (type == HID_TYPE_UNIT_EXPONENT) {
            unitExp = *value;
        }
        else if (type == HID_TYPE_UNIT) {
            unit = *value;
        }
        else if (type == HID_TYPE_UNIT_2) {
            unit = *value;
        }

        else if (inTouchTlc && depth == 2 && lastCollection == HID_USAGE_DIGITIZER_TOUCH_PAD  && lastUsage == HID_USAGE_DIGITIZER_FINGER) {//
            pDevContext->REPORTID_MULTITOUCH_COLLECTION = reportId;
            RegDebug(L"AnalyzeHidReportDescriptor REPORTID_MULTITOUCH_COLLECTION=", NULL, pDevContext->REPORTID_MULTITOUCH_COLLECTION);

            //这里计算单个报告数据包的手指数量用来后续判断报告模式及bHybrid_ReportingMode的赋值
            pDevContext->DeviceDescriptorFingerCount++;
            RegDebug(L"AnalyzeHidReportDescriptor DeviceDescriptorFingerCount=", NULL, pDevContext->DeviceDescriptorFingerCount);
        }
        else if (inMouseTlc && depth == 2 && lastCollection == HID_USAGE_GENERIC_MOUSE  && lastUsage == HID_USAGE_GENERIC_POINTER) {
            //下层的Mouse集合report本驱动并不会读取，只是作为输出到上层类驱动的Report使用
            pDevContext->REPORTID_MOUSE_COLLECTION = reportId;
            RegDebug(L"AnalyzeHidReportDescriptor REPORTID_MOUSE_COLLECTION=", NULL, pDevContext->REPORTID_MOUSE_COLLECTION);
        }
        else if (inConfigTlc && type == HID_TYPE_FEATURE && lastUsage == HID_USAGE_INPUT_MODE) {
            pDevContext->REPORTSIZE_INPUT_MODE = (reportSize + 7) / 8;//报告数据总长度
            pDevContext->REPORTID_INPUT_MODE = reportId;
           // RegDebug(L"AnalyzeHidReportDescriptor REPORTID_INPUT_MODE=", NULL, pDevContext->REPORTID_INPUT_MODE);
            //RegDebug(L"AnalyzeHidReportDescriptor REPORTSIZE_INPUT_MODE=", NULL, pDevContext->REPORTSIZE_INPUT_MODE);
            continue;
        }
        else if (inConfigTlc && type == HID_TYPE_FEATURE && lastUsage == HID_USAGE_SURFACE_SWITCH || lastUsage == HID_USAGE_BUTTON_SWITCH) {
            //默认标准规范为HID_USAGE_SURFACE_SWITCH与HID_USAGE_BUTTON_SWITCH各1bit组合低位成1个字节HID_USAGE_FUNCTION_SWITCH报告
            pDevContext->REPORTSIZE_FUNCTION_SWITCH = (reportSize + 7) / 8;//报告数据总长度
            pDevContext->REPORTID_FUNCTION_SWITCH = reportId;
            //RegDebug(L"AnalyzeHidReportDescriptor REPORTID_FUNCTION_SWITCH=", NULL, pDevContext->REPORTID_FUNCTION_SWITCH);
            //RegDebug(L"AnalyzeHidReportDescriptor REPORTSIZE_FUNCTION_SWITCH=", NULL, pDevContext->REPORTSIZE_FUNCTION_SWITCH);
            continue;
        }
        else if (inTouchTlc && type == HID_TYPE_FEATURE && lastUsage == HID_USAGE_CONTACT_COUNT_MAXIMUM || lastUsage == HID_USAGE_PAD_TYPE) {
            //默认标准规范为HID_USAGE_CONTACT_COUNT_MAXIMUM与HID_USAGE_PAD_TYPE各4bit组合低位成1个字节HID_USAGE_DEVICE_CAPS报告
            pDevContext->REPORTSIZE_DEVICE_CAPS = (reportSize + 7) / 8;//报告数据总长度
            pDevContext->REPORTID_DEVICE_CAPS = reportId;
            //RegDebug(L"AnalyzeHidReportDescriptor REPORTSIZE_DEVICE_CAPS=", NULL, pDevContext->REPORTSIZE_DEVICE_CAPS);
            //RegDebug(L"AnalyzeHidReportDescriptor REPORTID_DEVICE_CAPS=", NULL, pDevContext->REPORTID_DEVICE_CAPS);
            continue;
        }
        else if (inTouchTlc && type == HID_TYPE_FEATURE && lastUsage == HID_USAGE_PAGE_VENDOR_DEFINED_DEVICE_CERTIFICATION) {
            pDevContext->REPORTSIZE_PTPHQA = 256;
            pDevContext->REPORTID_PTPHQA = reportId;
            //RegDebug(L"AnalyzeHidReportDescriptor REPORTID_PTPHQA=", NULL, pDevContext->REPORTID_PTPHQA);
            continue;
        }
        else if (inTouchTlc && type == HID_TYPE_INPUT && lastUsage == HID_USAGE_X) {
            tp->physicalMax_X = physicalMax;
            tp->logicalMax_X = logicalMax;
            tp->unitExp = UnitExponent_Table[unitExp];
            tp->unit = unit;
            //RegDebug(L"AnalyzeHidReportDescriptor physicalMax_X=", NULL, tp->physicalMax_X);
            //RegDebug(L"AnalyzeHidReportDescriptor logicalMax_X=", NULL, tp->logicalMax_X);
            //RegDebug(L"AnalyzeHidReportDescriptor unitExp=", NULL, tp->unitExp);
            //RegDebug(L"AnalyzeHidReportDescriptor unit=", NULL, tp->unit);
            continue;
        }
        else if (inTouchTlc && type == HID_TYPE_INPUT && lastUsage == HID_USAGE_Y) {
            tp->physicalMax_Y = physicalMax;
            tp->logicalMax_Y = logicalMax;
            tp->unitExp = UnitExponent_Table[unitExp];
            tp->unit = unit;
            //RegDebug(L"AnalyzeHidReportDescriptor physicalMax_Y=", NULL, tp->physicalMax_Y);
            //RegDebug(L"AnalyzeHidReportDescriptor logicalMax_Y=", NULL, tp->logicalMax_Y);
            //RegDebug(L"AnalyzeHidReportDescriptor unitExp=", NULL, tp->unitExp);
           // RegDebug(L"AnalyzeHidReportDescriptor unit=", NULL, tp->unit);
            continue;
        }
    }

    //判断触摸板报告模式
    if (pDevContext->DeviceDescriptorFingerCount < 5) {//5个手指数据以下
        pDevContext->bHybrid_ReportingMode = TRUE;//混合报告模式确认
        RegDebug(L"AnalyzeHidReportDescriptor bHybrid_ReportingMode=", NULL, pDevContext->bHybrid_ReportingMode);
    }


    //计算保存触摸板尺寸分辨率等参数
    //转换为mm长度单位
    if (tp->unit == 0x11) {//cm长度单位
        tp->physical_Width_mm = tp->physicalMax_X * pow(10.0, tp->unitExp) * 10;
        tp->physical_Height_mm = tp->physicalMax_Y * pow(10.0, tp->unitExp) * 10;
    }
    else {//0x13为inch长度单位
        tp->physical_Width_mm = tp->physicalMax_X * pow(10.0, tp->unitExp) * 25.4;
        tp->physical_Height_mm = tp->physicalMax_Y * pow(10.0, tp->unitExp) * 25.4;
    }
    
    if (!tp->physical_Width_mm) {
        //RegDebug(L"AnalyzeHidReportDescriptor physical_Width_mm err", NULL, 0);
        return STATUS_UNSUCCESSFUL;
    }
    if (!tp->physical_Height_mm) {
        //RegDebug(L"AnalyzeHidReportDescriptor physical_Height_mm err", NULL, 0);
        return STATUS_UNSUCCESSFUL;
    }

    tp->TouchPad_DPMM_x = float(tp->logicalMax_X / tp->physical_Width_mm);//单位为dot/mm
    tp->TouchPad_DPMM_y = float(tp->logicalMax_Y / tp->physical_Height_mm);//单位为dot/mm
    //RegDebug(L"AnalyzeHidReportDescriptor TouchPad_DPMM_x=", NULL, (ULONG)tp->TouchPad_DPMM_x);
    //RegDebug(L"AnalyzeHidReportDescriptor TouchPad_DPMM_y=", NULL, (ULONG)tp->TouchPad_DPMM_y);

    //动态调整手指头大小常量
    tp->thumb_Width = 18;//手指头宽度,默认以中指18mm宽为基准
    tp->thumb_Scale = 1.0;//手指头尺寸缩放比例，
    tp->FingerMinDistance = 12 * tp->TouchPad_DPMM_x * tp->thumb_Scale;//定义有效的相邻手指最小距离
    tp->FingerClosedThresholdDistance = 16 * tp->TouchPad_DPMM_x * tp->thumb_Scale;//定义相邻手指合拢时的最小距离
    tp->FingerMaxDistance = tp->FingerMinDistance * 4;//定义有效的相邻手指最大距离(FingerMinDistance*4) 

    tp->PointerSensitivity_x = tp->TouchPad_DPMM_x / 25;
    tp->PointerSensitivity_y = tp->TouchPad_DPMM_y / 25;

    tp->StartY_TOP = (ULONG)(10 * tp->TouchPad_DPMM_y);////起点误触横线Y值为距离触摸板顶部10mm处的Y坐标
    ULONG halfwidth = (ULONG)(43.2 * tp->TouchPad_DPMM_x);//起点误触竖线X值为距离触摸板中心线左右侧43.2mm处的X坐标

    if (tp->logicalMax_X / 2 > halfwidth) {//触摸板宽度大于正常触摸起点区域宽度
        tp->StartX_LEFT = tp->logicalMax_X / 2 - halfwidth;
        tp->StartX_RIGHT = tp->logicalMax_X / 2 + halfwidth;
    }
    else {
        tp->StartX_LEFT = 0;
        tp->StartX_RIGHT = tp->logicalMax_X;
    }
    
    //RegDebug(L"AnalyzeHidReportDescriptor tp->StartTop_Y =", NULL, tp->StartY_TOP);
    //RegDebug(L"AnalyzeHidReportDescriptor tp->StartX_LEFT =", NULL, tp->StartX_LEFT);
    //RegDebug(L"AnalyzeHidReportDescriptor tp->StartX_RIGHT =", NULL, tp->StartX_RIGHT);

    RegDebug(L"AnalyzeHidReportDescriptor end", NULL, status);
    return status;
}


NTSTATUS
SendOriginalReport(PDEVICE_CONTEXT pDevContext, PVOID OriginalReport, size_t outputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;

    WDFREQUEST PtpRequest;
    WDFMEMORY  memory;

    status = WdfIoQueueRetrieveNextRequest(pDevContext->ReportQueue, &PtpRequest);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendOriginalReport WdfIoQueueRetrieveNextRequest failed", NULL, runtimes_ioControl);
        goto cleanup;
    }

    status = WdfRequestRetrieveOutputMemory(PtpRequest, &memory);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendOriginalReport WdfRequestRetrieveOutputMemory failed", NULL, runtimes_ioControl);
        goto exit;
    }

    status = WdfMemoryCopyFromBuffer(memory, 0, OriginalReport, outputBufferLength);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendOriginalReport WdfMemoryCopyFromBuffer failed", NULL, runtimes_ioControl);
        goto exit;
    }

    WdfRequestSetInformation(PtpRequest, outputBufferLength);
    RegDebug(L"SendOriginalReport ok", NULL, status);

exit:
    WdfRequestComplete(
        PtpRequest,
        status
    );

cleanup:
    RegDebug(L"SendOriginalReport end", NULL, status);
    return status;

}

NTSTATUS
SendPtpMultiTouchReport(PDEVICE_CONTEXT pDevContext, PVOID MultiTouchReport, size_t outputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;

    WDFREQUEST PtpRequest;
    WDFMEMORY  memory;

    status = WdfIoQueueRetrieveNextRequest(pDevContext->ReportQueue, &PtpRequest);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendPtpMultiTouchReport WdfIoQueueRetrieveNextRequest failed", NULL, runtimes_ioControl);
        goto cleanup;
    }

    status = WdfRequestRetrieveOutputMemory(PtpRequest, &memory);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendPtpMultiTouchReport WdfRequestRetrieveOutputMemory failed", NULL, runtimes_ioControl);
        goto exit;
    }

    status = WdfMemoryCopyFromBuffer(memory, 0, MultiTouchReport, outputBufferLength);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendPtpMultiTouchReport WdfMemoryCopyFromBuffer failed", NULL, runtimes_ioControl);
        goto exit;
    }

    WdfRequestSetInformation(PtpRequest, outputBufferLength);
    RegDebug(L"SendPtpMultiTouchReport ok", NULL, status);

exit:
    WdfRequestComplete(
        PtpRequest,
        status
    );

cleanup:
    RegDebug(L"SendPtpMultiTouchReport end", NULL, status);
    return status;

}

NTSTATUS
SendPtpMouseReport(PDEVICE_CONTEXT pDevContext, mouse_report_t* pMouseReport)
{
    NTSTATUS status = STATUS_SUCCESS;

    WDFREQUEST PtpRequest;
    WDFMEMORY  memory;
    size_t     outputBufferLength = sizeof(mouse_report_t);
    //RegDebug(L"SendPtpMouseReport pMouseReport=", pMouseReport, (ULONG)outputBufferLength);

    status = WdfIoQueueRetrieveNextRequest(pDevContext->ReportQueue, &PtpRequest);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendPtpMouseReport WdfIoQueueRetrieveNextRequest failed", NULL, runtimes_ioControl);
        goto cleanup;
    }

    status = WdfRequestRetrieveOutputMemory(PtpRequest, &memory);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendPtpMouseReport WdfRequestRetrieveOutputMemory failed", NULL, runtimes_ioControl);
        goto exit;
    }

    status = WdfMemoryCopyFromBuffer(memory, 0, pMouseReport, outputBufferLength);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"SendPtpMouseReport WdfMemoryCopyFromBuffer failed", NULL, runtimes_ioControl);
        goto exit;
    }

    WdfRequestSetInformation(PtpRequest, outputBufferLength);
    RegDebug(L"SendPtpMouseReport ok", NULL, status);

exit:
    WdfRequestComplete(
        PtpRequest,
        status
    );

cleanup:
    RegDebug(L"SendPtpMouseReport end", NULL, status);
    return status;

}


void MouseLikeTouchPad_parse(PDEVICE_CONTEXT pDevContext, PTP_REPORT* pPtpReport)
{
    NTSTATUS status = STATUS_SUCCESS;

    PTP_PARSER* tp = &pDevContext->tp_settings;

    //计算报告频率和时间间隔
    KeQueryTickCount(&tp->current_Ticktime);
    tp->ticktime_Interval.QuadPart = (tp->current_Ticktime.QuadPart - tp->last_Ticktime.QuadPart) * tp->tick_Count / 10000;//单位ms毫秒
    tp->TouchPad_ReportInterval = (float)tp->ticktime_Interval.LowPart;//触摸板报告间隔时间ms
    tp->last_Ticktime = tp->current_Ticktime;


    //保存当前手指坐标
    tp->currentFinger = *pPtpReport;
    UCHAR currentFinger_Count = tp->currentFinger.ContactCount;//当前触摸点数量
    UCHAR lastFinger_Count=tp->lastFinger.ContactCount; //上次触摸点数量
    //RegDebug(L"MouseLikeTouchPad_parse currentFinger_Count=", NULL, currentFinger_Count);
    //RegDebug(L"MouseLikeTouchPad_parse lastFinger_Count=", NULL, lastFinger_Count);

    UCHAR MAX_CONTACT_FINGER = PTP_MAX_CONTACT_POINTS;
    BOOLEAN allFingerDetached = TRUE;
    for (UCHAR i = 0; i < MAX_CONTACT_FINGER; i++) {//所有TipSwitch为0时判定为手指全部离开，因为最后一个点离开时ContactCount和Confidence始终为1不会置0。
        if (tp->currentFinger.Contacts[i].TipSwitch) {
            allFingerDetached = FALSE;
            currentFinger_Count = tp->currentFinger.ContactCount;//重新定义当前触摸点数量
            break;
        }
    }
    if (allFingerDetached) {
        currentFinger_Count = 0;
    }


    //初始化鼠标事件
    mouse_report_t mReport;
    mReport.report_id = FAKE_REPORTID_MOUSE;//FAKE_REPORTID_MOUSE//pDevContext->REPORTID_MOUSE_COLLECTION

    mReport.button = 0;
    mReport.dx = 0;
    mReport.dy = 0;
    mReport.h_wheel = 0;
    mReport.v_wheel = 0;

    BOOLEAN bMouse_LButton_Status = 0; //定义临时鼠标左键状态，0为释放，1为按下，每次都需要重置确保后面逻辑
    BOOLEAN bMouse_MButton_Status = 0; //定义临时鼠标中键状态，0为释放，1为按下，每次都需要重置确保后面逻辑
    BOOLEAN bMouse_RButton_Status = 0; //定义临时鼠标右键状态，0为释放，1为按下，每次都需要重置确保后面逻辑
    BOOLEAN bMouse_BButton_Status = 0; //定义临时鼠标Back后退键状态，0为释放，1为按下，每次都需要重置确保后面逻辑
    BOOLEAN bMouse_FButton_Status = 0; //定义临时鼠标Forward前进键状态，0为释放，1为按下，每次都需要重置确保后面逻辑

    //初始化当前触摸点索引号，跟踪后未再赋值的表示不存在了
    tp->nMouse_Pointer_CurrentIndex = -1;
    tp->nMouse_LButton_CurrentIndex = -1;
    tp->nMouse_RButton_CurrentIndex = -1;
    tp->nMouse_MButton_CurrentIndex = -1;
    tp->nMouse_Wheel_CurrentIndex = -1;


    //所有手指触摸点的索引号跟踪
    for (char i = 0; i < currentFinger_Count; i++) {
        if (tp->nMouse_Pointer_LastIndex != -1) {
            if (tp->lastFinger.Contacts[tp->nMouse_Pointer_LastIndex].ContactID == tp->currentFinger.Contacts[i].ContactID) {
                tp->nMouse_Pointer_CurrentIndex = i;//找到指针
                continue;//查找其他功能
            }
        }

        if (tp->nMouse_Wheel_LastIndex != -1) {
            if (tp->lastFinger.Contacts[tp->nMouse_Wheel_LastIndex].ContactID == tp->currentFinger.Contacts[i].ContactID) {
                tp->nMouse_Wheel_CurrentIndex = i;//找到滚轮辅助键
                continue;//查找其他功能
            }
        }

        if (tp->nMouse_LButton_LastIndex != -1) {
            if (tp->lastFinger.Contacts[tp->nMouse_LButton_LastIndex].ContactID == tp->currentFinger.Contacts[i].ContactID) {
                bMouse_LButton_Status = 1; //找到左键，
                tp->nMouse_LButton_CurrentIndex = i;//赋值左键触摸点新索引号
                continue;//查找其他功能
            }
        }

        if (tp->nMouse_RButton_LastIndex != -1) {
            if (tp->lastFinger.Contacts[tp->nMouse_RButton_LastIndex].ContactID == tp->currentFinger.Contacts[i].ContactID) {
                bMouse_RButton_Status = 1; //找到右键，
                tp->nMouse_RButton_CurrentIndex = i;//赋值右键触摸点新索引号
                continue;//查找其他功能
            }
        }

        if (tp->nMouse_MButton_LastIndex != -1) {
            if (tp->lastFinger.Contacts[tp->nMouse_MButton_LastIndex].ContactID == tp->currentFinger.Contacts[i].ContactID) {
                bMouse_MButton_Status = 1; //找到中键，
                tp->nMouse_MButton_CurrentIndex = i;//赋值中键触摸点新索引号
                continue;//查找其他功能
            }
        }     
    }

    //RegDebug(L"MouseLikeTouchPad_parse traced currentFinger_Count=", NULL, currentFinger_Count);
    //RegDebug(L"MouseLikeTouchPad_parse pDevContext->bHybrid_ReportingMode=", NULL, pDevContext->bHybrid_ReportingMode);

    if (tp->currentFinger.IsButtonClicked) {//触摸板下沿物理按键功能,切换触控板灵敏度/滚轮模式开关等参数设置,需要进行离开判定，因为按键报告会一直发送直到释放
        tp->bPhysicalButtonUp = FALSE;//物理键是否释放标志
        RegDebug(L"MouseLikeTouchPad_parse bPhysicalButtonUp FALSE", NULL, FALSE);
        //准备设置触摸板下沿物理按键相关参数
        if (currentFinger_Count == 1) {//单指重按触控板左下角物理键为鼠标的后退功能键，单指重按触控板右下角物理键为鼠标的前进功能键，单指重按触控板下沿中间物理键为调节鼠标灵敏度（慢/中等/快3段灵敏度），
            if (tp->currentFinger.Contacts[0].ContactID == 0 && tp->currentFinger.Contacts[0].Confidence && tp->currentFinger.Contacts[0].TipSwitch\
                && tp->currentFinger.Contacts[0].Y > (tp->logicalMax_Y / 2) && tp->currentFinger.Contacts[0].X < tp->StartX_LEFT) {//首个触摸点坐标在左下角
                bMouse_BButton_Status = 1;//鼠标侧面的后退键按下
            }
            else if (tp->currentFinger.Contacts[0].ContactID == 0 && tp->currentFinger.Contacts[0].Confidence && tp->currentFinger.Contacts[0].TipSwitch\
                && tp->currentFinger.Contacts[0].Y > (tp->logicalMax_Y / 2) && tp->currentFinger.Contacts[0].X > tp->StartX_RIGHT) {//首个触摸点坐标在右下角
                bMouse_FButton_Status = 1;//鼠标侧面的前进键按下
            }
            else {//切换鼠标DPI灵敏度，放在物理键释放时执行判断

            }

        }
    }
    else {
        if (!tp->bPhysicalButtonUp) {
            tp->bPhysicalButtonUp = TRUE;
            RegDebug(L"MouseLikeTouchPad_parse bPhysicalButtonUp TRUE", NULL, TRUE);
            if (currentFinger_Count == 1) {//单指重按触控板下沿中间物理键为调节鼠标灵敏度（慢/中等/快3段灵敏度），鼠标的后退/前进功能键不需要判断会自动释放)，
                if (tp->currentFinger.Contacts[0].ContactID == 0 && tp->currentFinger.Contacts[0].Confidence && tp->currentFinger.Contacts[0].TipSwitch\
                    && tp->currentFinger.Contacts[0].Y > (tp->logicalMax_Y/2) && tp->currentFinger.Contacts[0].X >  tp->StartX_LEFT && tp->currentFinger.Contacts[0].X < tp->StartX_RIGHT) {//首个触摸点坐标在触摸板下沿中间
                    //切换鼠标DPI灵敏度
                    SetNextSensitivity(pDevContext);//循环设置灵敏度
                }          
            }
            else if (currentFinger_Count == 2) {//双指重按触控板下沿物理键时设置为开启/关闭双指滚轮功能
                //不采用3指滚轮方式因为判断区分双指先接触的操作必须加大时间阈值使得延迟太高不合适,玩游戏较少使用到滚轮功能可选择关闭切换可以极大降低玩游戏时的误操作率，所以采取开启关闭滚轮方案兼顾日常操作和游戏

                pDevContext->bWheelDisabled = !pDevContext->bWheelDisabled;
                RegDebug(L"MouseLikeTouchPad_parse bWheelDisabled=", NULL, pDevContext->bWheelDisabled);
                if (!pDevContext->bWheelDisabled) {//开启滚轮功能时同时也恢复滚轮实现方式为触摸板双指滑动手势
                    if (pDevContext->bHybrid_ReportingMode) {//混合报告模式默认滚轮方式为模仿鼠标(双指滑动手势还存在一些卡顿问题体验不好)
                        pDevContext->bWheelScrollMode = TRUE;
                    }
                    else {
                        pDevContext->bWheelScrollMode = FALSE;//默认初始值为触摸板双指滑动手势
                    }
                    RegDebug(L"MouseLikeTouchPad_parse bWheelScrollMode=", NULL, pDevContext->bWheelScrollMode);
                }

                RegDebug(L"MouseLikeTouchPad_parse bPhysicalButtonUp currentFinger_Count=", NULL, currentFinger_Count);
            }
            else if (currentFinger_Count == 3) {//三指重按触控板下沿物理键时设置为切换滚轮模式bWheelScrollMode，定义鼠标滚轮实现方式，TRUE为模仿鼠标滚轮，FALSE为触摸板双指滑动手势
                //因为日常操作滚轮更常用，所以关闭滚轮功能的状态不保存到注册表，电脑重启或休眠唤醒后恢复滚轮功能
                //因为触摸板双指滑动手势的滚轮模式更常用，所以模仿鼠标的滚轮模式状态不保存到注册表，电脑重启或休眠唤醒后恢复到双指滑动手势的滚轮模式
                pDevContext->bWheelScrollMode = !pDevContext->bWheelScrollMode;
                RegDebug(L"MouseLikeTouchPad_parse bWheelScrollMode=", NULL, pDevContext->bWheelScrollMode);

                //切换滚轮实现方式的同时也开启滚轮功能方便用户
                pDevContext->bWheelDisabled = FALSE;
                RegDebug(L"MouseLikeTouchPad_parse bWheelDisabled=", NULL, pDevContext->bWheelDisabled);


                RegDebug(L"MouseLikeTouchPad_parse bPhysicalButtonUp currentFinger_Count=", NULL, currentFinger_Count);
            }
            else if (currentFinger_Count == 4 && !pDevContext->bHybrid_ReportingMode) {//四指按压触控板物理按键时切换仿鼠标式触摸板与windows原版的PTP精确式触摸板操作方式
                //混合报告模式的触控板存在诸多问题未解决所以没有此功能
                //因为原版触控板操作方式只是临时使用所以不保存到注册表，电脑重启或休眠唤醒后恢复到仿鼠标式触摸板模式
                // 原版的PTP精确式触摸板操作方式时发送报告在本函数外部执行不需要浪费资源解析，切换回仿鼠标式触摸板模式也在本函数外部判断
                pDevContext->bMouseLikeTouchPad_Mode = FALSE;
                RegDebug(L"MouseLikeTouchPad_parse bMouseLikeTouchPad_Mode=", NULL, pDevContext->bMouseLikeTouchPad_Mode);
  
                RegDebug(L"MouseLikeTouchPad_parse bPhysicalButtonUp currentFinger_Count=", NULL, currentFinger_Count);
            }
            
        }
    }

    //开始鼠标事件逻辑判定
    //注意多手指非同时快速接触触摸板时触摸板报告可能存在一帧中同时新增多个触摸点的情况所以不能用当前只有一个触摸点作为定义指针的判断条件
    if (tp->nMouse_Pointer_LastIndex == -1 && currentFinger_Count > 0) {//鼠标指针、左键、右键、中键都未定义,
        //指针触摸点压力、接触面长宽比阈值特征区分判定手掌打字误触和正常操作,压力越小接触面长宽比阈值越大、长度阈值越小
        for (UCHAR i = 0; i < currentFinger_Count; i++) {
            if (tp->currentFinger.Contacts[i].ContactID == 0 && tp->currentFinger.Contacts[i].Confidence && tp->currentFinger.Contacts[i].TipSwitch\
                && tp->currentFinger.Contacts[i].Y > tp->StartY_TOP && tp->currentFinger.Contacts[i].X > tp->StartX_LEFT && tp->currentFinger.Contacts[i].X < tp->StartX_RIGHT) {//起点坐标在误触横竖线以内
                tp->nMouse_Pointer_CurrentIndex = i;  //首个触摸点作为指针
                tp->MousePointer_DefineTime = tp->current_Ticktime;//定义当前指针起始时间
                break;
            }
        }
    }
    else if (tp->nMouse_Pointer_CurrentIndex == -1 && tp->nMouse_Pointer_LastIndex != -1) {//指针消失
        tp->bMouse_Wheel_Mode = FALSE;//结束滚轮模式
        tp->bMouse_Wheel_Mode_JudgeEnable = TRUE;//开启滚轮判别

        tp->bGestureCompleted = TRUE;//手势模式结束,但tp->bPtpReportCollection不要重置待其他代码来处理

        tp->nMouse_Pointer_CurrentIndex = -1;
        tp->nMouse_LButton_CurrentIndex = -1;
        tp->nMouse_RButton_CurrentIndex = -1;
        tp->nMouse_MButton_CurrentIndex = -1;
        tp->nMouse_Wheel_CurrentIndex = -1;
    }
    else if (tp->nMouse_Pointer_CurrentIndex != -1 && !tp->bMouse_Wheel_Mode) {  //指针已定义的非滚轮事件处理
        //查找指针左侧或者右侧是否有手指作为滚轮模式或者按键模式，当指针左侧/右侧的手指按下时间与指针手指定义时间间隔小于设定阈值时判定为鼠标滚轮否则为鼠标按键，这一规则能有效区别按键与滚轮操作,但鼠标按键和滚轮不能一起使用
        //按键定义后会跟踪坐标所以左键和中键不能滑动食指互相切换需要抬起食指后进行改变，左键/中键/右键按下的情况下不能转变为滚轮模式，
        LARGE_INTEGER MouseButton_Interval;
        MouseButton_Interval.QuadPart = (tp->current_Ticktime.QuadPart - tp->MousePointer_DefineTime.QuadPart) * tp->tick_Count / 10000;//单位ms毫秒
        float Mouse_Button_Interval = (float)MouseButton_Interval.LowPart;//指针左右侧的手指按下时间与指针定义起始时间的间隔ms
        
        if (currentFinger_Count > 1) {//触摸点数量超过1才需要判断按键操作
            for (char i = 0; i < currentFinger_Count; i++) {
                if (i == tp->nMouse_Pointer_CurrentIndex || i == tp->nMouse_LButton_CurrentIndex || i == tp->nMouse_RButton_CurrentIndex || i == tp->nMouse_MButton_CurrentIndex || i == tp->nMouse_Wheel_CurrentIndex) {//i为正值所以无需检查索引号是否为-1
                    continue;  // 已经定义的跳过
                }
                float dx = (float)(tp->currentFinger.Contacts[i].X - tp->currentFinger.Contacts[tp->nMouse_Pointer_CurrentIndex].X);
                float dy = (float)(tp->currentFinger.Contacts[i].Y - tp->currentFinger.Contacts[tp->nMouse_Pointer_CurrentIndex].Y);
                float distance = sqrt(dx * dx + dy * dy);//触摸点与指针的距离

                BOOLEAN isWheel = FALSE;//滚轮模式成立条件初始化重置，注意bWheelDisabled与bMouse_Wheel_Mode_JudgeEnable的作用不同，不能混淆
                if (!pDevContext->bWheelDisabled) {//滚轮功能开启时
                    // 指针左右侧有手指按下并且与指针手指起始定义时间间隔小于阈值，指针被定义后区分滚轮操作只需判断一次直到指针消失，后续按键操作判断不会被时间阈值约束使得响应速度不受影响
                    isWheel = tp->bMouse_Wheel_Mode_JudgeEnable && abs(distance) > tp->FingerMinDistance && abs(distance) < tp->FingerMaxDistance && Mouse_Button_Interval < ButtonPointer_Interval_MSEC;
                }
                
                if (isWheel) {//滚轮模式条件成立
                    tp->bMouse_Wheel_Mode = TRUE;  //开启滚轮模式
                    tp->bMouse_Wheel_Mode_JudgeEnable = FALSE;//关闭滚轮判别

                    tp->bGestureCompleted = FALSE; //手势操作结束标志,但tp->bPtpReportCollection不要重置待其他代码来处理

                    tp->nMouse_Wheel_CurrentIndex = i;//滚轮辅助参考手指索引值
                    //手指变化瞬间时电容可能不稳定指针坐标突发性漂移需要忽略
                    tp->JitterFixStartTime = tp->current_Ticktime;//抖动修正开始计时
                    tp->Scroll_TotalDistanceX = 0;//累计滚动位移量重置
                    tp->Scroll_TotalDistanceY = 0;//累计滚动位移量重置


                    tp->nMouse_LButton_CurrentIndex = -1;
                    tp->nMouse_RButton_CurrentIndex = -1;
                    tp->nMouse_MButton_CurrentIndex = -1;
                    break;
                }
                else {//前面滚轮模式条件判断已经排除了所以不需要考虑与指针手指起始定义时间间隔，
                    if (tp->nMouse_MButton_CurrentIndex == -1 && abs(distance) > tp->FingerMinDistance && abs(distance) < tp->FingerClosedThresholdDistance && dx < 0) {//指针左侧有并拢的手指按下
                        bMouse_MButton_Status = 1; //找到中键
                        tp->nMouse_MButton_CurrentIndex = i;//赋值中键触摸点新索引号
                        continue;  //继续找其他按键，食指已经被中键占用所以原则上左键已经不可用
                    }
                    else if (tp->nMouse_LButton_CurrentIndex == -1 && abs(distance) > tp->FingerClosedThresholdDistance && abs(distance) < tp->FingerMaxDistance && dx < 0) {//指针左侧有分开的手指按下
                        bMouse_LButton_Status = 1; //找到左键
                        tp->nMouse_LButton_CurrentIndex = i;//赋值左键触摸点新索引号
                        continue;  //继续找其他按键
                    }
                    else if (tp->nMouse_RButton_CurrentIndex == -1 && abs(distance) > tp->FingerMinDistance && abs(distance) < tp->FingerMaxDistance && dx > 0) {//指针右侧有手指按下
                        bMouse_RButton_Status = 1; //找到右键
                        tp->nMouse_RButton_CurrentIndex = i;//赋值右键触摸点新索引号
                        continue;  //继续找其他按键
                    }
                }

            }
        }
        
        //鼠标指针位移设置
        if (currentFinger_Count != lastFinger_Count) {//手指变化瞬间时电容可能不稳定指针坐标突发性漂移需要忽略
            tp->JitterFixStartTime = tp->current_Ticktime;//抖动修正开始计时
        }
        else {
            LARGE_INTEGER FixTimer;
            FixTimer.QuadPart = (tp->current_Ticktime.QuadPart - tp->JitterFixStartTime.QuadPart) * tp->tick_Count / 10000;//单位ms毫秒
            float JitterFixTimer = (float)FixTimer.LowPart;//当前抖动时间计时

            float STABLE_INTERVAL;
            if (tp->nMouse_MButton_CurrentIndex != -1) {//中键状态下手指并拢的抖动修正值区别处理
                STABLE_INTERVAL = STABLE_INTERVAL_FingerClosed_MSEC;
            }
            else {
                STABLE_INTERVAL = STABLE_INTERVAL_FingerSeparated_MSEC;
            }

            float px = (float)(tp->currentFinger.Contacts[tp->nMouse_Pointer_CurrentIndex].X - tp->lastFinger.Contacts[tp->nMouse_Pointer_LastIndex].X) / tp->thumb_Scale;
            float py = (float)(tp->currentFinger.Contacts[tp->nMouse_Pointer_CurrentIndex].Y - tp->lastFinger.Contacts[tp->nMouse_Pointer_LastIndex].Y) / tp->thumb_Scale;

            if (JitterFixTimer < STABLE_INTERVAL) {//触摸点稳定前修正
                if (tp->nMouse_LButton_CurrentIndex != -1 || tp->nMouse_RButton_CurrentIndex != -1 || tp->nMouse_MButton_CurrentIndex != -1) {//有按键时修正，单指针时不需要使得指针更精确
                    if (abs(px) <= Jitter_Offset) {//指针轻微抖动修正
                        px = 0;
                    }
                    if (abs(py) <= Jitter_Offset) {//指针轻微抖动修正
                        py = 0;
                    }
                }
            }

            double xx= pDevContext->MouseSensitivity_Value * px / tp->PointerSensitivity_x;
            double yy = pDevContext->MouseSensitivity_Value* py / tp->PointerSensitivity_y;
            mReport.dx = (UCHAR)xx;
            mReport.dy = (UCHAR)yy;
            if (xx > 0.5 && xx < 1) {//慢速精细移动指针修正
                mReport.dx = 1;
            }
            if (yy > 0.5 && yy < 1) {//慢速精细移动指针修正
                mReport.dy = 1;
            }
            

        }
    }
    else if (tp->nMouse_Pointer_CurrentIndex != -1 && tp->bMouse_Wheel_Mode) {//滚轮操作模式，触摸板双指滑动、三指四指手势也归为此模式下的特例设置一个手势状态开关供后续判断使用
        if (!pDevContext->bWheelScrollMode || currentFinger_Count >2) {//触摸板双指滑动手势模式，三指四指手势也归为此模式
            tp->bPtpReportCollection = TRUE;//发送PTP触摸板集合报告，后续再做进一步判断
        }
        else {
            //鼠标指针位移设置
            LARGE_INTEGER FixTimer;
            FixTimer.QuadPart = (tp->current_Ticktime.QuadPart - tp->JitterFixStartTime.QuadPart) * tp->tick_Count / 10000;//单位ms毫秒
            float JitterFixTimer = (float)FixTimer.LowPart;//当前抖动时间计时

            float px = (float)(tp->currentFinger.Contacts[tp->nMouse_Pointer_CurrentIndex].X - tp->lastFinger.Contacts[tp->nMouse_Pointer_LastIndex].X) / tp->thumb_Scale;
            float py = (float)(tp->currentFinger.Contacts[tp->nMouse_Pointer_CurrentIndex].Y - tp->lastFinger.Contacts[tp->nMouse_Pointer_LastIndex].Y) / tp->thumb_Scale;

            if (JitterFixTimer < STABLE_INTERVAL_FingerClosed_MSEC) {//只需在触摸点稳定前修正
                if (abs(px) <= Jitter_Offset) {//指针轻微抖动修正
                    px = 0;
                }
                if (abs(py) <= Jitter_Offset) {//指针轻微抖动修正
                    py = 0;
                }
            }

            int direction_hscale = 1;//滚动方向缩放比例
            int direction_vscale = 1;//滚动方向缩放比例

            if (abs(px) > abs(py) / 4) {//滚动方向稳定性修正
                direction_hscale = 1;
                direction_vscale = 8;
            }
            if (abs(py) > abs(px) / 4) {//滚动方向稳定性修正
                direction_hscale = 8;
                direction_vscale = 1;
            }

            px = px / direction_hscale;
            py = py / direction_vscale;

            px = (float)(pDevContext->MouseSensitivity_Value * px / tp->PointerSensitivity_x);
            py = (float)(pDevContext->MouseSensitivity_Value * py / tp->PointerSensitivity_y);

            tp->Scroll_TotalDistanceX += px;//累计滚动位移量
            tp->Scroll_TotalDistanceY += py;//累计滚动位移量

            //判断滚动量
            if (abs(tp->Scroll_TotalDistanceX) > SCROLL_OFFSET_THRESHOLD_X) {//位移量超过阈值
                int h = (int)(abs(tp->Scroll_TotalDistanceX) / SCROLL_OFFSET_THRESHOLD_X);
                mReport.h_wheel = (char)(tp->Scroll_TotalDistanceX > 0 ? h : -h);//滚动行数

                float r = abs(tp->Scroll_TotalDistanceX) - SCROLL_OFFSET_THRESHOLD_X * h;// 滚动位移量余数绝对值
                tp->Scroll_TotalDistanceX = tp->Scroll_TotalDistanceX > 0 ? r : -r;//滚动位移量余数
            }
            if (abs(tp->Scroll_TotalDistanceY) > SCROLL_OFFSET_THRESHOLD_Y) {//位移量超过阈值
                int v = (int)(abs(tp->Scroll_TotalDistanceY) / SCROLL_OFFSET_THRESHOLD_Y);
                mReport.v_wheel = (char)(tp->Scroll_TotalDistanceY > 0 ? v : -v);//滚动行数

                float r = abs(tp->Scroll_TotalDistanceY) - SCROLL_OFFSET_THRESHOLD_Y * v;// 滚动位移量余数绝对值
                tp->Scroll_TotalDistanceY = tp->Scroll_TotalDistanceY > 0 ? r : -r;//滚动位移量余数
            }
        }
        
    }
    else {
        //其他组合无效
    }


    if (tp->bPtpReportCollection) {//触摸板集合，手势模式判断
        if (!tp->bMouse_Wheel_Mode) {//以指针手指释放为滚轮模式结束标志，下一帧bPtpReportCollection会设置FALSE所以只会发送一次构造的手势结束报告
            tp->bPtpReportCollection = FALSE;//PTP触摸板集合报告模式结束
            tp->bGestureCompleted = TRUE;//结束手势操作，该数据和bMouse_Wheel_Mode区分开了，因为bGestureCompleted可能会比bMouse_Wheel_Mode提前结束
            RegDebug(L"MouseLikeTouchPad_parse bPtpReportCollection bGestureCompleted0", NULL, status);

            //构造全部手指释放的临时数据包,TipSwitch域归零，windows手势操作结束时需要手指离开的点xy坐标数据
            PTP_REPORT CompletedGestureReport;
            RtlCopyMemory(&CompletedGestureReport, &tp->currentFinger, sizeof(PTP_REPORT));
            for (int i = 0; i < currentFinger_Count; i++) {
                CompletedGestureReport.Contacts[i].TipSwitch = 0;
            }

            //发送ptp报告
            status = SendPtpMultiTouchReport(pDevContext, &CompletedGestureReport, sizeof(PTP_REPORT));
            if (!NT_SUCCESS(status)) {
                RegDebug(L"MouseLikeTouchPad_parse SendPtpMultiTouchReport CompletedGestureReport failed", NULL, status);
            }

        }
        else if(tp->bMouse_Wheel_Mode && currentFinger_Count == 1 && !tp->bGestureCompleted) {//滚轮模式未结束并且剩下指针手指留在触摸板上,需要配合bGestureCompleted标志判断使得构造的手势结束报告只发送一次
            tp->bPtpReportCollection = FALSE;//PTP触摸板集合报告模式结束
            tp->bGestureCompleted = TRUE;//提前结束手势操作，该数据和bMouse_Wheel_Mode区分开了，因为bGestureCompleted可能会比bMouse_Wheel_Mode提前结束
            RegDebug(L"MouseLikeTouchPad_parse bPtpReportCollection bGestureCompleted1", NULL, status);

            //构造指针手指释放的临时数据包,TipSwitch域归零，windows手势操作结束时需要手指离开的点xy坐标数据
            PTP_REPORT CompletedGestureReport2;
            RtlCopyMemory(&CompletedGestureReport2, &tp->currentFinger, sizeof(PTP_REPORT));
            CompletedGestureReport2.Contacts[0].TipSwitch = 0;

            //发送ptp报告
            status = SendPtpMultiTouchReport(pDevContext, &CompletedGestureReport2, sizeof(PTP_REPORT));
            if (!NT_SUCCESS(status)) {
                RegDebug(L"MouseLikeTouchPad_parse SendPtpMultiTouchReport CompletedGestureReport2 failed", NULL, status);
            }
        }

        if (!tp->bGestureCompleted) {//手势未结束，正常发送报告
            RegDebug(L"MouseLikeTouchPad_parse bPtpReportCollection bGestureCompleted2", NULL, status);
            //发送ptp报告
            status = SendPtpMultiTouchReport(pDevContext, pPtpReport, sizeof(PTP_REPORT));
            if (!NT_SUCCESS(status)) {
                RegDebug(L"MouseLikeTouchPad_parse SendPtpMultiTouchReport failed", NULL, status);
            }
        }
    }
    else{//发送MouseCollection
        mReport.button = bMouse_LButton_Status + (bMouse_RButton_Status << 1) + (bMouse_MButton_Status << 2) + (bMouse_BButton_Status << 3) + (bMouse_FButton_Status << 4);  //左中右后退前进键状态合成
        //发送鼠标报告
        status = SendPtpMouseReport(pDevContext, &mReport);
        if (!NT_SUCCESS(status)) {
            RegDebug(L"MouseLikeTouchPad_parse SendPtpMouseReport failed", NULL, status);
        }
    }
    

    //保存下一轮所有触摸点的初始坐标及功能定义索引号
    tp->lastFinger = tp->currentFinger;

    lastFinger_Count = currentFinger_Count;
    tp->nMouse_Pointer_LastIndex = tp->nMouse_Pointer_CurrentIndex;
    tp->nMouse_LButton_LastIndex = tp->nMouse_LButton_CurrentIndex;
    tp->nMouse_RButton_LastIndex = tp->nMouse_RButton_CurrentIndex;
    tp->nMouse_MButton_LastIndex = tp->nMouse_MButton_CurrentIndex;
    tp->nMouse_Wheel_LastIndex = tp->nMouse_Wheel_CurrentIndex;

}



static __forceinline short abs(short x)
{
    if (x < 0)return -x;
    return x;
}

void MouseLikeTouchPad_parse_init(PDEVICE_CONTEXT pDevContext)
{
    PTP_PARSER* tp= &pDevContext->tp_settings;

    tp->nMouse_Pointer_CurrentIndex = -1; //定义当前鼠标指针触摸点坐标的数据索引号，-1为未定义
    tp->nMouse_LButton_CurrentIndex = -1; //定义当前鼠标左键触摸点坐标的数据索引号，-1为未定义
    tp->nMouse_RButton_CurrentIndex = -1; //定义当前鼠标右键触摸点坐标的数据索引号，-1为未定义
    tp->nMouse_MButton_CurrentIndex = -1; //定义当前鼠标中键触摸点坐标的数据索引号，-1为未定义
    tp->nMouse_Wheel_CurrentIndex = -1; //定义当前鼠标滚轮辅助参考手指触摸点坐标的数据索引号，-1为未定义

   tp-> nMouse_Pointer_LastIndex = -1; //定义上次鼠标指针触摸点坐标的数据索引号，-1为未定义
   tp->nMouse_LButton_LastIndex = -1; //定义上次鼠标左键触摸点坐标的数据索引号，-1为未定义
   tp->nMouse_RButton_LastIndex = -1; //定义上次鼠标右键触摸点坐标的数据索引号，-1为未定义
   tp->nMouse_MButton_LastIndex = -1; //定义上次鼠标中键触摸点坐标的数据索引号，-1为未定义
   tp->nMouse_Wheel_LastIndex = -1; //定义上次鼠标滚轮辅助参考手指触摸点坐标的数据索引号，-1为未定义

   pDevContext->bWheelDisabled = FALSE;//默认初始值为开启滚轮操作功能
   if (pDevContext->bHybrid_ReportingMode) {//混合报告模式默认滚轮方式为模仿鼠标(双指滑动手势还存在一些卡顿问题体验不好)
       pDevContext->bWheelScrollMode = TRUE;
   }
   else {
       pDevContext->bWheelScrollMode = FALSE;//默认初始值为触摸板双指滑动手势
   }


   tp->bMouse_Wheel_Mode = FALSE;
   tp->bMouse_Wheel_Mode_JudgeEnable = TRUE;//开启滚轮判别

   tp->bGestureCompleted = FALSE; //手势操作结束标志
   tp->bPtpReportCollection = FALSE;//默认鼠标集合

   RtlZeroMemory(&tp->lastFinger, sizeof(PTP_REPORT));
   RtlZeroMemory(&tp->currentFinger, sizeof(PTP_REPORT));

    tp->Scroll_TotalDistanceX = 0;
    tp->Scroll_TotalDistanceY = 0;

    tp->tick_Count = KeQueryTimeIncrement();
    KeQueryTickCount(&tp->last_Ticktime);

    tp->bPhysicalButtonUp = TRUE;
}


void SetNextSensitivity(PDEVICE_CONTEXT pDevContext)
{  
    UCHAR ms_idx = pDevContext->MouseSensitivity_Index;// MouseSensitivity_Normal;//MouseSensitivity_Slow//MouseSensitivity_FAST

    ms_idx++;
    if (ms_idx == 3) {//灵敏度循环设置
        ms_idx = 0;
    }

    //保存注册表灵敏度设置数值
    NTSTATUS status = SetRegisterMouseSensitivity(pDevContext, ms_idx);//MouseSensitivityTable存储表的序号值
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"SetNextSensitivity SetRegisterMouseSensitivity err", NULL, status);
        return;
    }

    pDevContext->MouseSensitivity_Index = ms_idx;
    pDevContext->MouseSensitivity_Value = MouseSensitivityTable[ms_idx];
    RegDebug(L"SetNextSensitivity pDevContext->MouseSensitivity_Index", NULL, pDevContext->MouseSensitivity_Index);

    RegDebug(L"SetNextSensitivity ok", NULL, status);
}


NTSTATUS SetRegisterMouseSensitivity(PDEVICE_CONTEXT pDevContext, ULONG ms_idx)//保存设置到注册表
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device = pDevContext->FxDevice;

    DECLARE_CONST_UNICODE_STRING(ValueNameString, L"MouseSensitivity_Index");

    WDFKEY hKey = NULL;

    status = WdfDeviceOpenRegistryKey(
        device,
        PLUGPLAY_REGKEY_DEVICE,//1
        KEY_WRITE,
        WDF_NO_OBJECT_ATTRIBUTES,
        &hKey);

    if (NT_SUCCESS(status)) {
        status = WdfRegistryAssignULong(hKey, &ValueNameString, ms_idx);
        if (!NT_SUCCESS(status)) {
            RegDebug(L"SetRegisterMouseSensitivity WdfRegistryAssignULong err", NULL, status);
            return status;
        }    
    }

    if (hKey) {
        WdfObjectDelete(hKey);
    }

    RegDebug(L"SetRegisterMouseSensitivity ok", NULL, status);
    return status;
}



NTSTATUS GetRegisterMouseSensitivity(PDEVICE_CONTEXT pDevContext, ULONG* ms_idx)//从注册表读取设置
{

    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device = pDevContext->FxDevice;

    WDFKEY hKey = NULL;
    *ms_idx = 0;

    DECLARE_CONST_UNICODE_STRING(ValueNameString, L"MouseSensitivity_Index");

    status = WdfDeviceOpenRegistryKey(
        device,
        PLUGPLAY_REGKEY_DEVICE,//1
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &hKey);

    if (NT_SUCCESS(status))
    {
        status = WdfRegistryQueryULong(hKey, &ValueNameString, ms_idx);
    }
    else {
        RegDebug(L"GetRegisterMouseSensitivity err", NULL, status);
    }

    if (hKey) {
        WdfObjectDelete(hKey);
    }

    RegDebug(L"GetRegisterMouseSensitivity end", NULL, status);
    return status;
}


void  SetAAPThreshold(PDEVICE_CONTEXT pDevContext)
{
    if (pDevContext->bSetAAPThresholdOK) {
        return;
    }

    pDevContext->bSetAAPThresholdOK = TRUE;

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    pDevContext->bFoundRegCurrentUserSID = FALSE;//当前用户SID是否找到
    UNICODE_STRING SidUserName;
    status = GetCurrentUserSID(pDevContext, &SidUserName);//获取当前用户SID
    if (!NT_SUCCESS(status)) {
        RegDebug(L"OnD0Entry GetCurrentUserSID SidUserName err", NULL, status);
    }
    else {
        status = SetRegisterAAPThreshold(&SidUserName);//设置Touchpad sensitivity触摸板敏感度AAPThreshold为最高Maximum sensitivity
        if (!NT_SUCCESS(status)) {
            RegDebug(L"OnD0Entry SetRegisterAAPThreshold SidUserName err", NULL, status);
        }

        //保存SID
        pDevContext->strCurrentUserSID.Buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, SidUserName.Length, HIDI2C_POOL_TAG);
        if (pDevContext->strCurrentUserSID.Buffer == NULL)
        {
            status = STATUS_UNSUCCESSFUL;
            RegDebug(L"GetCurrentUserSID pDevContext->strCurrentUserSID.Buffer err", NULL, status);
        }
        pDevContext->strCurrentUserSID.Length = SidUserName.Length;//必须设置长度，否则数据会错误
        pDevContext->strCurrentUserSID.MaximumLength = SidUserName.Length;
        RtlCopyUnicodeString(&pDevContext->strCurrentUserSID, &SidUserName);//注意顺序在设置长度之后，否则数据会错误
    }
    if (SidUserName.Buffer != NULL)
    {
        RtlFreeUnicodeString(&SidUserName);
    }

    return;
}

NTSTATUS  SetRegisterAAPThreshold(PUNICODE_STRING pSidReg)//触控板AAP意外激活防护Accidental Activation Prevention功能，设置Touchpad sensitivity触摸板敏感度AAPThreshold为最高Maximum sensitivity
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    if (pSidReg->Buffer == NULL || pSidReg->Length <= 20)
    {
        RegDebug(L"SetRegisterAAPThreshold pSidReg err", NULL, status);
        return status;
    }

    //初始化注册表项
    UNICODE_STRING stringKey;
    UNICODE_STRING strAAPThreshold;

    WCHAR CurrentUserbuf[256];
    UNICODE_STRING RegCurrentUserLocation, RegUserLocation;
    RtlInitUnicodeString(&RegUserLocation, L"\\Registry\\User\\");//HKET__USERS位置
    RtlInitUnicodeString(&strAAPThreshold, L"\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad");//AAPThreshold键位置后段

    RtlInitEmptyUnicodeString(&RegCurrentUserLocation, CurrentUserbuf, 256 * sizeof(WCHAR));

    RtlCopyUnicodeString(&RegCurrentUserLocation, &RegUserLocation);
    RtlAppendUnicodeStringToString(&RegCurrentUserLocation, pSidReg);//得到HKET_CURRENT_USER位置
    RegDebug(L"SetRegisterAAPThreshold HKET_CURRENT_USER =", RegCurrentUserLocation.Buffer, RegCurrentUserLocation.Length);

    RtlAppendUnicodeStringToString(&RegCurrentUserLocation, &strAAPThreshold);//得到完整AAPThreshold键位置
    RtlInitUnicodeString(&stringKey, RegCurrentUserLocation.Buffer);
    RegDebug(L"SetRegisterAAPThreshold AAPThreshold Key=", stringKey.Buffer, stringKey.Length);

    //RtlInitUnicodeString(&stringKey, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad");//AAPDisabled键位置

    //初始化OBJECT_ATTRIBUTES结构
    OBJECT_ATTRIBUTES  ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes, &stringKey, OBJ_KERNEL_HANDLE, NULL, NULL);//OBJ_KERNEL_HANDLE//OBJ_CASE_INSENSITIVE对大小写敏感

    //创建注册表项
    HANDLE hKey;
    ULONG Des;
    status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, &Des);//KEY_ALL_ACCESS//KEY_READ| KEY_WRITE
    if (NT_SUCCESS(status))
    {
        if (Des == REG_CREATED_NEW_KEY)
        {
            KdPrint(("新建注册表项！\n"));
        }
        else
        {
            KdPrint(("要创建的注册表项已经存在！\n"));
        }
    }
    else {
        RegDebug(L"SetRegisterAAPThreshold ZwCreateKey err", NULL, status);//STATUS_OBJECT_NAME_NOT_FOUND
        return status;
    }


    //初始化valueName
    UNICODE_STRING valueName;
    RtlInitUnicodeString(&valueName, L"AAPThreshold");

    //设置REG_DWORD键值
    ULONG AAPThreshold = 0;//设置触摸板敏感度为最高，仿鼠标式触摸板驱动的防止误触功能依赖此参数设置，0 == Most sensitive,1 == High sensitivity,2 == Medium sensitivity(default),3 == Low sensitivity
    status = ZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &AAPThreshold, 4);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("设置REG_DWORD键值失败！\n"));
        RegDebug(L"SetRegisterAAPThreshold err", NULL, status);
    }

    ////初始化valueName2
    //UNICODE_STRING valueName2;
    //RtlInitUnicodeString(&valueName2, L"AAPDisabled");

    ////设置REG_DWORD键值
    //ULONG AAPDisabled = 1;//关闭触摸板敏感度功能
    //status = ZwSetValueKey(hKey, &valueName2, 0, REG_DWORD, &AAPDisabled, 4);
    //if (!NT_SUCCESS(status))
    //{
    //    KdPrint(("设置REG_DWORD键值失败！\n"));
    //}

    ZwFlushKey(hKey);
    //关闭注册表句柄
    ZwClose(hKey);

    RegDebug(L"SetRegisterAAPThreshold end", NULL, status);
    return status;
}


NTSTATUS  GetCurrentUserSID(PDEVICE_CONTEXT pDevContext, PUNICODE_STRING pSidReg)
{
    //RegDebug(L"GetCurrentUserSID start", NULL, 0);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE hRegHandle = NULL;
    OBJECT_ATTRIBUTES objSid;
    ULONG uRetLength = 0;
    PKEY_FULL_INFORMATION pkfiQuery = NULL;
    PKEY_BASIC_INFORMATION pbiEnumKey = NULL;
    ULONG uIndex = 0;
    UNICODE_STRING uniKeyName;
    WCHAR ProfileListBuf[256];
    WCHAR RegSidBuf[256];
    UNICODE_STRING RegProfileList, strSaveSidRegLocation, RegSid;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    ULONG udefaultData = 0;
    ULONG uQueryValue;

    
    RtlZeroMemory(paramTable, sizeof(paramTable));

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"RefCount";
    paramTable[0].EntryContext = &uQueryValue;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &udefaultData;
    paramTable[0].DefaultLength = sizeof(ULONG);

    RtlInitUnicodeString(&strSaveSidRegLocation, L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
 
    RtlInitEmptyUnicodeString(&RegProfileList, ProfileListBuf, 256 * sizeof(WCHAR));
    RtlCopyUnicodeString(&RegProfileList, &strSaveSidRegLocation);

    RtlInitEmptyUnicodeString(&RegSid, RegSidBuf, 256 * sizeof(WCHAR));

    InitializeObjectAttributes(&objSid, &strSaveSidRegLocation, OBJ_KERNEL_HANDLE, NULL, NULL);//OBJ_KERNEL_HANDLE//OBJ_CASE_INSENSITIVE

    status = ZwOpenKey(&hRegHandle, KEY_ALL_ACCESS, &objSid);//KEY_ALL_ACCESS//KEY_READ
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"GetCurrentUserSID ZwOpenKey err", NULL, status);
        goto END;
    }

    //ZwQueryKey ZwEnumKey Get Sid
    status = ZwQueryKey(hRegHandle, KeyFullInformation, NULL, 0, &uRetLength);
    if (status != STATUS_BUFFER_TOO_SMALL)//必须是该错误
    {
        RegDebug(L"GetCurrentUserSID ZwQueryKey err", NULL, status);
        goto END;
    }

    pkfiQuery = (PKEY_FULL_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, uRetLength, HIDI2C_POOL_TAG);
    if (pkfiQuery == NULL)
    {
        RegDebug(L"GetCurrentUserSID ExAllocatePoolWithTag pkfiQuery err", NULL, status);
        goto END;
    }

    RtlZeroMemory(pkfiQuery, uRetLength);
    status = ZwQueryKey(hRegHandle, KeyFullInformation, pkfiQuery, uRetLength, &uRetLength);
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"GetCurrentUserSID ZwQueryKey pkfiQuery err", NULL, status);
        goto END;
    }

    RegDebug(L"GetCurrentUser ZwQueryKey pkfiQuery ok", NULL, status);
    for (uIndex = 0; uIndex < pkfiQuery->SubKeys; uIndex++)
    {
        status = ZwEnumerateKey(hRegHandle, uIndex, KeyBasicInformation, NULL, 0, &uRetLength);
        if (status != STATUS_BUFFER_TOO_SMALL)//必须是该错误
        {
            RegDebug(L"GetCurrentUserSID ZwEnumerateKey err", NULL, status);
            goto END;
        }

        pbiEnumKey = (PKEY_BASIC_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, uRetLength, HIDI2C_POOL_TAG);
        if (pbiEnumKey == NULL)
        {
            RegDebug(L"GetCurrentUserSID ExAllocatePoolWithTag pbiEnumKey err", NULL, status);
            goto END;
        }

        RtlZeroMemory(pbiEnumKey, uRetLength);
        status = ZwEnumerateKey(hRegHandle, uIndex, KeyBasicInformation, pbiEnumKey, uRetLength, &uRetLength);
        if (!NT_SUCCESS(status))
        {
            RegDebug(L"GetCurrentUserSID ZwEnumerateKey pbiEnumKey err", NULL, status);
            goto END;
        }

        RegDebug(L"GetCurrentUserSID ZwEnumerateKey pbiEnumKey ok", NULL, status);
        uniKeyName.Length = (USHORT)pbiEnumKey->NameLength;
        uniKeyName.MaximumLength = (USHORT)pbiEnumKey->NameLength;
        uniKeyName.Buffer = pbiEnumKey->Name;
        //RegDebug(L"GetCurrentUser pbiEnumKey->Name=", pbiEnumKey->Name, pbiEnumKey->NameLength);

        if (pbiEnumKey->NameLength > 20)
        {
            BOOLEAN* pFoundSID = &pDevContext->bFoundRegCurrentUserSID;
            if (!(*pFoundSID)) {//第一个找到的SID先保存，如果后面有多用户SID会被替换
                *pFoundSID = TRUE;//找到当前用户SID
                RtlCopyUnicodeString(&RegSid, &uniKeyName);//SID内容会变化所以用定长数值buf的RegSid
                RegDebug(L"GetCurrentUserSID RegSid =", RegSid.Buffer, RegSid.Length);
            }

            //判断多用户下登录的当前用户SID
            RtlAppendUnicodeStringToString(&RegProfileList, &uniKeyName);
            RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, RegProfileList.Buffer, paramTable, NULL, NULL);
            if (uQueryValue > 0)//当前登录用户
            {
                RtlCopyUnicodeString(&RegSid, &uniKeyName);//SID内容会变化所以用定长数值buf的RegSid//替换初始用户SID
                RegDebug(L"GetCurrentUserSID new RegSid =", RegSid.Buffer, RegSid.Length);
            }
        }

        RtlCopyUnicodeString(&RegProfileList, &strSaveSidRegLocation);

    }  

    if (RegSid.Length <=20)
    {
        status = STATUS_UNSUCCESSFUL;
        RegDebug(L"GetCurrentUserSID RegSid.Length err", NULL, status);
        goto END;
    }

    //赋值最终SID常量
    pSidReg->Buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, RegSid.Length, HIDI2C_POOL_TAG);
    if (pSidReg->Buffer == NULL)
    {
        status = STATUS_UNSUCCESSFUL;
        RegDebug(L"GetCurrentUserSID pSidReg->Buffer err", NULL, status);
        goto END;
    }

    pSidReg->Length = RegSid.Length;//必须设置长度，否则pSidReg数据会错误
    pSidReg->MaximumLength = RegSid.Length;
    RtlCopyUnicodeString(pSidReg, &RegSid);//注意顺序在设置长度之后，否则pSidReg数据会错误

    //RegDebug(L"GetCurrentUserSID RegSid2 =", RegSid.Buffer, RegSid.Length);
    RegDebug(L"GetCurrentUserSID pSidReg =", pSidReg->Buffer, pSidReg->Length);


END:
    if (pbiEnumKey != NULL) {
        ExFreePoolWithTag(pbiEnumKey, HIDI2C_POOL_TAG);
        pbiEnumKey = NULL;
    }
    if (pkfiQuery != NULL) {
        ExFreePoolWithTag(pkfiQuery, HIDI2C_POOL_TAG);
        pkfiQuery = NULL;
    }
    if (hRegHandle != NULL)
    {
        ZwClose(hRegHandle);
        hRegHandle = NULL;
    }

    return status;
}

//
VOID
OnDeviceResetNotificationRequestCancel(
    _In_  WDFREQUEST FxRequest
)
/*++
Routine Description:

    OnDeviceResetNotificationRequestCancel is the EvtRequestCancel routine for
    Device Reset Notification requests. It's called by the framework when HIDCLASS
    is cancelling the Device Reset Notification request that is pending in HIDI2C
    driver. It basically completes the pending request with STATUS_CANCELLED.

Arguments:

    FxRequest - Handle to a framework request object being cancelled.

Return Value:

    None

--*/
{
    PDEVICE_CONTEXT pDeviceContext = GetDeviceContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(FxRequest)));
    NTSTATUS        status = STATUS_CANCELLED;

    //
    // Hold the spinlock to sync up with other threads who
    // reads and then writes to DeviceResetNotificationRequest won't run
    // into a problem while holding the same spinlock.
    //
    WdfSpinLockAcquire(pDeviceContext->DeviceResetNotificationSpinLock);

    //
    // It may have been cleared by either of the following. So we just clear
    // the matched pDeviceContext->DeviceResetNotificationRequest.
    //
    // - ISR (when Device Initiated Reset occurs)
    // - OnIoStop (when device is being removed or leaving D0)
    // 
    if (pDeviceContext->DeviceResetNotificationRequest == FxRequest)
    {
        pDeviceContext->DeviceResetNotificationRequest = NULL;
    }

    WdfSpinLockRelease(pDeviceContext->DeviceResetNotificationSpinLock);

    //
    // Simply complete the request here.
    //
    WdfRequestComplete(FxRequest, status);
}
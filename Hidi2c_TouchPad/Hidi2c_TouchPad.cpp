#include "Hidi2c_TouchPad.h"
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

//#pragma alloc_text(PAGE, OnD0Exit)

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
    InitializeObjectAttributes(&ObjectAttributes, &stringKey, OBJ_CASE_INSENSITIVE, NULL, NULL);

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
	UNREFERENCED_PARAMETER(pDriverObject);

	RegDebug(L"OnDriverUnload", NULL, 0);
}


NTSTATUS OnDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(Driver);

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
	status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->ReportQueue);
	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfIoQueueCreate ReportQueue failed", NULL, status);
		return status;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
	status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->IdleQueue);
	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfIoQueueCreate IdleQueue failed", NULL, status);
		return status;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
	status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &pDevContext->ResetNotificationQueue);
	if (!NT_SUCCESS(status)) {
		RegDebug(L"WdfIoQueueCreate ResetNotificationQueue failed", NULL, status);
		return status;
	}

	//RegDebug(L"OnDeviceAdd ok", NULL, 0);
	return STATUS_SUCCESS;

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

    HID_REPORT_TYPE hidReportType;
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
        case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        {
            RegDebug(L"IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST", NULL, runtimes_ioControl);
            status = HidSendIdleNotification(pDevContext, Request, &requestPendingFlag);
            break;
        }

        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        {
            RegDebug(L"IOCTL_HID_GET_DEVICE_DESCRIPTOR", NULL, runtimes_ioControl);

            WDFMEMORY memory;
            status = WdfRequestRetrieveOutputMemory(Request, &memory);
            if (!NT_SUCCESS(status)) {
                RegDebug(L"IOCTL_HID_GET_DEVICE_DESCRIPTOR WdfRequestRetrieveOutputMemory err", NULL, runtimes_ioControl);
                break;
            }

            status = WdfMemoryCopyFromBuffer(memory, 0, (PVOID)&DefaultHidDescriptor, DefaultHidDescriptor.bLength);
            if (!NT_SUCCESS(status)) {
                RegDebug(L"IOCTL_HID_GET_DEVICE_DESCRIPTOR WdfMemoryCopyFromBuffer err", NULL, runtimes_ioControl);
                break;
            }
            ////
            WdfRequestSetInformation(Request, DefaultHidDescriptor.bLength);

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

            LONG outlen = DefaultHidDescriptor.DescriptorList[0].wReportLength;
            status = WdfMemoryCopyFromBuffer(memory, 0, (PVOID)MouseReportDescriptor, outlen); //TouchpadReportDescriptor  MouseReportDescriptor//AmtPtpSpiFamily3aReportDescriptor
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
            RegDebug(L"IOCTL_HID_READ_REPORT", NULL, runtimes_ioControl);
        
            status = HidReadReport(pDevContext, Request, &requestPendingFlag);
            if (requestPendingFlag) {
                //测试
                if (pDevContext->SetFeatureReady == TRUE) {

                    pDevContext->SetFeatureReady = FALSE;
                    status = HidSetFeature(pDevContext, PTP_FEATURE_INPUT_COLLECTION);//设置触摸板input mode
                    if (!NT_SUCCESS(status)) {
                         RegDebug(L"HidSetFeature PTP_SET_FEATURE_INPUT_COLLECTION err", NULL, status);
                    }
                    status = HidSetFeature(pDevContext, PTP_FEATURE_SELECTIVE_REPORTING);//设置触摸板SELECTIVE_REPORTING
                    if (!NT_SUCCESS(status)) {
                        RegDebug(L"HidSetFeature PTP_SET_FEATURE_SELECTIVE_REPORTING err", NULL, status);
                    }
                    RegDebug(L"IOCTL_HID_READ_REPORT SetFeatureReady", NULL, runtimes_ioControl);
                }                

                return;
            }
            break;
        }   
       
        case IOCTL_HID_GET_FEATURE:
        {
            RegDebug(L"IOCTL_HID_GET_FEATURE", NULL, runtimes_ioControl);
            status = PtpReportFeatures(
                Device,
                Request
            );
            break;
        }    
        
        case IOCTL_HID_GET_STRING:
        {
            RegDebug(L"IOCTL_HID_GET_STRING", NULL, runtimes_ioControl);
            status = STATUS_NOT_IMPLEMENTED;
            //status = HidGetString(pDevContext, Request);//代码会死机
            break;
        }  

        case IOCTL_HID_GET_INPUT_REPORT:
            RegDebug(L"IOCTL_HID_GET_INPUT_REPORT", NULL, runtimes_ioControl);
            status = STATUS_NOT_SUPPORTED;
            //hidReportType = ReportTypeInput;//1
            //status = HidGetReport(pDevContext, Request, hidReportType);
            break;

        case IOCTL_HID_SET_OUTPUT_REPORT:
            RegDebug(L"IOCTL_HID_SET_OUTPUT_REPORT", NULL, runtimes_ioControl);
            status = HidSetReport(pDevContext, Request, ReportTypeOutput);//2
            break;

       /* case IOCTL_HID_DEVICERESET_NOTIFICATION:
        {
            RegDebug(L"IOCTL_HID_DEVICERESET_NOTIFICATION", NULL, runtimes_ioControl);
            WdfIoQueueGetState(pDevContext->ResetNotificationQueue, &IoControlCode, NULL);
            status = WdfRequestForwardToIoQueue(Request, pDevContext->ResetNotificationQueue);
            break;
        }*/

        default:
        {
            status = STATUS_NOT_SUPPORTED;
            RegDebug(L"STATUS_NOT_SUPPORTED", NULL, IoControlCode);
            RegDebug(L"STATUS_NOT_SUPPORTED FUNCTION_FROM_CTL_CODE", NULL, FUNCTION_FROM_CTL_CODE(IoControlCode));
        }

    }

    WdfRequestComplete(Request, status);
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


NTSTATUS OnPostInterruptsEnabled(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
    runtimes_OnPostInterruptsEnabled++;
    //RegDebug(L"OnPostInterruptsEnabled runtimes_OnPostInterruptsEnabled ", NULL, runtimes_OnPostInterruptsEnabled);

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);
    if (PreviousState == WdfPowerDeviceD3Final) {
        //RegDebug(L"OnPostInterruptsEnabled HidReset", NULL, runtimes_OnPostInterruptsEnabled);
        status = HidReset(pDevContext);
    }
    UNREFERENCED_PARAMETER(pDevContext);
    //RegDebug(L"OnPostInterruptsEnabled end", NULL, runtimes_OnPostInterruptsEnabled);
    return status;
}


NTSTATUS OnSelfManagedIoSuspend(WDFDEVICE Device)
{
    runtimes_OnSelfManagedIoSuspend++;

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(Device);
    UNREFERENCED_PARAMETER(pDevContext);

    RegDebug(L"OnSelfManagedIoSuspend end", NULL, runtimes_OnSelfManagedIoSuspend);
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
    pDevContext->inputModeReportId = 0;
    pDevContext->inputModeReportSize = 0;
    pDevContext->funswitchReportId = 0;
    pDevContext->funswitchReportSize = 0;
    pDevContext->HidReportDescriptorSaved = FALSE;
    pDevContext->MouseSensitivityIndex = 1;//默认初始值为序号1
    pDevContext->MouseSensitivityValue = 1.0;//默认初始值为1.0

    ULONG ms_idx;
    status = GetRegisterMouseSensitivity(pDevContext, &ms_idx);
    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)//     ((NTSTATUS)0xC0000034L)
        {
            RegDebug(L"OnPrepareHardware GetRegisterMouseSensitivity STATUS_OBJECT_NAME_NOT_FOUND", NULL, status);
            status = SetRegisterMouseSensitivity(pDevContext, 2);//默认设置为序号1，存储值为2
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
        pDevContext->MouseSensitivityIndex = (ms_idx & 0x03) - 1;//存储的序号值为1/2/3计算更方便
        pDevContext->MouseSensitivityValue = MouseSensitivityTable[pDevContext->MouseSensitivityIndex];
        RegDebug(L"OnPrepareHardware GetRegisterMouseSensitivity MouseSensitivityIndex=", NULL, pDevContext->MouseSensitivityIndex);
    }
    

    RegDebug(L"OnPrepareHardware ok", NULL, status);
    return STATUS_SUCCESS;
}

NTSTATUS OnReleaseHardware(_In_  WDFDEVICE FxDevice, _In_  WDFCMRESLIST  FxResourcesTranslated)
{
    UNREFERENCED_PARAMETER(FxResourcesTranslated);

    PDEVICE_CONTEXT pDevContext = GetDeviceContext(FxDevice);

    if (pDevContext->SpbRequest) {
        WdfObjectDelete(pDevContext->SpbRequest);
    }

    if (pDevContext->SpbIoTarget) {
        WdfObjectDelete(pDevContext->SpbIoTarget);
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

    PowerIdleIrpCompletion(pDevContext);

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

    pDevContext->SetFeatureReady = TRUE;
    MouseLikeTouchPad_parse_init(pDevContext);

    //RegDebug(L"OnD0Entry ok", NULL, 0);
    return STATUS_SUCCESS;
}

NTSTATUS OnD0Exit(_In_ WDFDEVICE FxDevice, _In_ WDF_POWER_DEVICE_STATE FxPreviousState)
{
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(FxDevice);

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
        RegDebug(L"SpbWriteWrite failed parameters DataFirst DataLengthFirst", NULL, status);

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
    //WDFDEVICE device = pDevContext->FxDevice;

    *(PUSHORT)data = 256;//0x0100

    WdfInterruptAcquireLock(pDevContext->ReadInterrupt);

    WdfIoQueueStopSynchronously(pDevContext->IoctlQueue);

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
    //LONG StrIDs = *(PULONG)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    
    USHORT stringSizeCb = 0;
    PWSTR string;

    USHORT wStrID = *(PUSHORT)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    switch (wStrID) {
    case HID_STRING_ID_IMANUFACTURER:
        stringSizeCb = sizeof(MANUFACTURER_ID_STRING);
        string = MANUFACTURER_ID_STRING;
        break;
    case HID_STRING_ID_IPRODUCT:
        stringSizeCb = sizeof(PRODUCT_ID_STRING);
        string = PRODUCT_ID_STRING;
        break;
    case HID_STRING_ID_ISERIALNUMBER:
        stringSizeCb = sizeof(SERIAL_NUMBER_STRING);
        string = SERIAL_NUMBER_STRING;
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        RegDebug(L"GetString: unkown string id", NULL, wStrID);
        goto exit;
    }

    ULONG bufferlength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
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

    memmove(pIrp->UserBuffer, string, stringSizeCb);
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
HidReadReport(
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
                    RegDebug(L"HidGetReport ReportSize", NULL, (ULONG)ReportSize);
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

        PBYTE pBuf = (PBYTE)pInputReportBuffer + 2;
        PTP_REPORT ptpReport = *(PPTP_REPORT)pBuf;

        //RegDebug(L"OnInterruptIsr PTP_REPORT.ReportID", NULL, ptpReport.ReportID);
        RegDebug(L"OnInterruptIsr PTP_REPORT.IsButtonClicked", NULL, ptpReport.IsButtonClicked);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.ScanTime", NULL, ptpReport.ScanTime);
        RegDebug(L"OnInterruptIsr PTP_REPORT.ContactCount", NULL, ptpReport.ContactCount);
   
        RegDebug(L"OnInterruptIsr PTP_REPORT..Contacts[0].Confidence ", NULL, ptpReport.Contacts[0].Confidence);
        RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].ContactID ", NULL, ptpReport.Contacts[0].ContactID);
        RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].TipSwitch ", NULL, ptpReport.Contacts[0].TipSwitch);
        //RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].Padding ", NULL, ptpReport.Contacts[0].Padding);
        RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].X ", NULL, ptpReport.Contacts[0].X);
        RegDebug(L"OnInterruptIsr PTP_REPORT.Contacts[0].Y ", NULL, ptpReport.Contacts[0].Y);


        mouse_report_t mReport;
        mReport.report_id = REPORTID_MOUSE;

        //MouseLikeTouchPad解析器
        MouseLikeTouchPad_parse(pDevContext, &ptpReport, &mReport);

        status = SendPtpMouseReport(pDevContext, &mReport);
        if (!NT_SUCCESS(status)) {
            RegDebug(L"OnInterruptIsr SendPtpMouseReport failed", NULL, runtimes_ioControl);
        }

        //RegDebug(L"SendReport end", NULL, status);
        goto exit;

    }

    if (pDevContext->HostInitiatedResetActive == TRUE) {

        pDevContext->HostInitiatedResetActive = FALSE;
        WdfIoQueueStart(pDevContext->IoctlQueue);

        RegDebug(L"OnInterruptIsr ok", NULL, status);
    }
    else {

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
            RegDebug(L"WdfIoQueueRetrieveNextRequest failed to find idle notification request in IdleQueue", NULL, status);
        }
        else
        {
            WdfRequestComplete(request, status);
            RegDebug(L"Completed idle notification Request from IdleQueue", NULL, status);
        }
    }

    RegDebug(L"PowerIdleIrpCompletion end", NULL, status);
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

    switch (pHidPacket->reportId)
    {
    case REPORTID_DEVICE_CAPS:
    {
        ReportSize = sizeof(PTP_DEVICE_CAPS_FEATURE_REPORT);
        if (pHidPacket->reportBufferLen < ReportSize) {
            Status = STATUS_INVALID_BUFFER_SIZE;

            goto exit;
        }

        PPTP_DEVICE_CAPS_FEATURE_REPORT capsReport = (PPTP_DEVICE_CAPS_FEATURE_REPORT)pHidPacket->reportBuffer;

        capsReport->MaximumContactPoints = PTP_MAX_CONTACT_POINTS;
        capsReport->ButtonType = PTP_BUTTON_TYPE_CLICK_PAD;
        capsReport->ReportID = REPORTID_DEVICE_CAPS;
        RegDebug(L"PtpGetFeatures pHidPacket->reportId REPORTID_DEVICE_CAPS ButtonType", NULL, capsReport->ButtonType = PTP_BUTTON_TYPE_CLICK_PAD);

        break;
    }
    case REPORTID_PTPHQA:
    {
        // Size sanity check
        ReportSize = sizeof(PTP_DEVICE_HQA_CERTIFICATION_REPORT);
        if (pHidPacket->reportBufferLen < ReportSize)
        {
            Status = STATUS_INVALID_BUFFER_SIZE;

            goto exit;
        }

        PPTP_DEVICE_HQA_CERTIFICATION_REPORT certReport = (PPTP_DEVICE_HQA_CERTIFICATION_REPORT)pHidPacket->reportBuffer;

        *certReport->CertificationBlob = DEFAULT_PTP_HQA_BLOB;
        certReport->ReportID = REPORTID_PTPHQA;
        RegDebug(L"PtpGetFeatures pHidPacket->reportId REPORTID_PTPHQA", NULL, pHidPacket->reportId);

        break;
    }
    default:
    {

        Status = STATUS_NOT_SUPPORTED;
        RegDebug(L"PtpGetFeatures pHidPacket->reportId STATUS_NOT_SUPPORTED", NULL, pHidPacket->reportId);
        goto exit;
    }
    }
    RegDebug(L"PtpGetFeatures STATUS_SUCCESS pDeviceContext->PtpInputOn", NULL, pDevContext->PtpInputOn);
exit:

    return Status;
}



NTSTATUS
HidSetFeature(
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
    UCHAR reportID;
    UCHAR reportData;
    UCHAR reportDataSize;

    if (SetType== PTP_FEATURE_INPUT_COLLECTION) {
        reportID = pDevContext->inputModeReportId;////reportID//yoga14s为0x04,matebook为0x03
        reportDataSize = pDevContext->inputModeReportSize;
        reportData = PTP_COLLECTION_WINDOWS;
    }
    else {//SetType== PTP_FEATURE_SELECTIVE_REPORTING
        reportID = pDevContext->funswitchReportId;////reportID//yoga14s为0x06,matebook为0x05
        reportDataSize = pDevContext->funswitchReportSize;
        reportData = PTP_SELECTIVE_REPORT_Button_Surface_ON;
    }

    if (reportID >= 0xFu) {
        HeaderLength = 3;
        pReportHeaderData = HeaderData3;
        *(PUSHORT)pReportHeaderData = 0x033F;//0x0330 | 0xF
        pReportHeaderData[2] = reportID;
    }
    else {
        HeaderLength = 2;
        pReportHeaderData = HeaderData2;
        *(PUSHORT)pReportHeaderData = 0x0330 | reportID;   
    }
    RegDebug(L"HidSetReport pReportHeaderData=", pReportHeaderData, HeaderLength);

    reportLength = 3 + reportDataSize;
    pFeatureReportData = (PBYTE)ExAllocatePoolWithTag(NonPagedPoolNx, reportLength, HIDI2C_POOL_TAG);
    if (!pFeatureReportData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        RegDebug(L"HidSetReport pFeatureReportData STATUS_INSUFFICIENT_RESOURCES", NULL, status);
        goto exit;
    }

    *(PUSHORT)pFeatureReportData = reportLength;
    pFeatureReportData[2] = reportID;//REPORTID_REPORTMODE或者REPORTID_FUNCTION_SWITCH
    pFeatureReportData[3] = reportData;//PTP_COLLECTION_WINDOWS或者PTP_SELECTIVE_REPORT_Button_Surface_ON
    RegDebug(L"HidSetReport pFeatureReportData=", pFeatureReportData, reportLength);

    RegisterAddressFirst = pDevContext->HidSettings.CommandRegisterAddress;
    RegisterAddressSecond = pDevContext->HidSettings.DataRegisterAddress;

    status = SpbWriteWrite(pDevContext->SpbIoTarget, RegisterAddressFirst, pReportHeaderData, HeaderLength, RegisterAddressSecond, pFeatureReportData, reportLength);
    if (!NT_SUCCESS(status)) {
        RegDebug(L"HidSetFeature SpbWriteWrite err", NULL, status);
    }

exit:
    RegDebug(L"HidSetFeature end", NULL, status);
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
    BYTE reportCount = 0;
    BYTE lastUsage = 0;
    bool inConfigTlc = false;
    bool inTouchTlc = false;
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

        if (type == HID_TYPE_COLLECTION) {
            depth++;
            if (depth == 1 && usagePage == HID_USAGE_PAGE_DIGITIZERS && lastUsage == HID_USAGE_CONFIGURATION) {
                inConfigTlc = true;
                RegDebug(L"AnalyzeHidReportDescriptor inConfigTlc", NULL, 0);
            }
            else if (depth == 1 && usagePage == HID_USAGE_PAGE_DIGITIZERS && lastUsage == HID_USAGE_TOUCHPAD) {
                inTouchTlc = true;
                RegDebug(L"AnalyzeHidReportDescriptor inTouchTlc", NULL, 0);
            }
        }
        else if (type == HID_TYPE_END_COLLECTION) {
            depth--;
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
        //
        else if (type == LOGICAL_MINIMUM) {
            logicalMax = *value;
        }
        else if (type == LOGICAL_MAXIMUM_2) {
            logicalMax = *(PUSHORT)value;
        }
        else if (type == PHYSICAL_MAXIMUM) {
            physicalMax = *value;
        }
        else if (type == PHYSICAL_MAXIMUM_2) {
            physicalMax= *(PUSHORT)value;
        }
        else if (type == UNIT_EXPONENT) {
            unitExp = *value;
        }
        else if (type == UNIT) {
            unit = *value;
        }
        else if (type == UNIT_2) {
            unit = *value;
        }

        else if (inConfigTlc && type == HID_TYPE_FEATURE && lastUsage == HID_USAGE_INPUT_MODE) {
            pDevContext->inputModeReportSize = (reportSize + 7) / 8;
            pDevContext->inputModeReportId = reportId;
            RegDebug(L"AnalyzeHidReportDescriptor inputModeReportId=", NULL, pDevContext->inputModeReportId);
            RegDebug(L"AnalyzeHidReportDescriptor inputModeReportSize=", NULL, pDevContext->inputModeReportSize);
            continue;
        }
        else if (inConfigTlc && type == HID_TYPE_FEATURE && lastUsage == SURFACE_SWITCH) {
            pDevContext->funswitchReportSize = (reportSize + 7) / 8;
            pDevContext->funswitchReportId = reportId;
            RegDebug(L"AnalyzeHidReportDescriptor funswitchReportId=", NULL, pDevContext->funswitchReportId);
            RegDebug(L"AnalyzeHidReportDescriptor funswitchReportSize=", NULL, pDevContext->funswitchReportSize);
            continue;
        }
        else if (inTouchTlc && type == INPUT && lastUsage == HID_USAGE_X) {
            tp->physicalMax_X = physicalMax;
            tp->logicalMax_X = logicalMax;
            tp->unitExp = UnitExponent_Table[unitExp];
            tp->unit = unit;
            RegDebug(L"AnalyzeHidReportDescriptor physicalMax_X=", NULL, tp->physicalMax_X);
            RegDebug(L"AnalyzeHidReportDescriptor logicalMax_X=", NULL, tp->logicalMax_X);
            RegDebug(L"AnalyzeHidReportDescriptor unitExp=", NULL, tp->unitExp);
            RegDebug(L"AnalyzeHidReportDescriptor unit=", NULL, tp->unit);
            continue;
        }
        else if (inTouchTlc && type == INPUT && lastUsage == HID_USAGE_Y) {
            tp->physicalMax_Y = physicalMax;
            tp->logicalMax_Y = logicalMax;
            tp->unitExp = UnitExponent_Table[unitExp];
            tp->unit = unit;
            RegDebug(L"AnalyzeHidReportDescriptor physicalMax_Y=", NULL, tp->physicalMax_Y);
            RegDebug(L"AnalyzeHidReportDescriptor logicalMax_Y=", NULL, tp->logicalMax_Y);
            RegDebug(L"AnalyzeHidReportDescriptor unitExp=", NULL, tp->unitExp);
            RegDebug(L"AnalyzeHidReportDescriptor unit=", NULL, tp->unit);
            continue;
        }
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
        RegDebug(L"AnalyzeHidReportDescriptor physical_Width_mm err", NULL, 0);
        return STATUS_UNSUCCESSFUL;
    }
    if (!tp->physical_Height_mm) {
        RegDebug(L"AnalyzeHidReportDescriptor physical_Height_mm err", NULL, 0);
        return STATUS_UNSUCCESSFUL;
    }

    tp->TouchPad_DPMM_x = float(tp->logicalMax_X / tp->physical_Width_mm);//单位为dot/mm
    tp->TouchPad_DPMM_y = float(tp->logicalMax_Y / tp->physical_Height_mm);//单位为dot/mm
    RegDebug(L"AnalyzeHidReportDescriptor TouchPad_DPMM_x=", NULL, (ULONG)tp->TouchPad_DPMM_x);
    RegDebug(L"AnalyzeHidReportDescriptor TouchPad_DPMM_y=", NULL, (ULONG)tp->TouchPad_DPMM_y);

    //动态调整手指头大小常量
    tp->thumb_width = 18;//手指头宽度,默认以中指18mm宽为基准
    tp->thumb_scale = 1.0;//手指头尺寸缩放比例，
    tp->FingerMinDistance = 12 * tp->TouchPad_DPMM_x * tp->thumb_scale;//定义有效的相邻手指最小距离
    tp->FingerClosedThresholdDistance = 16 * tp->TouchPad_DPMM_x * tp->thumb_scale;//定义相邻手指合拢时的最小距离
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
    
    RegDebug(L"AnalyzeHidReportDescriptor tp->StartTop_Y =", NULL, tp->StartY_TOP);
    RegDebug(L"AnalyzeHidReportDescriptor tp->StartX_LEFT =", NULL, tp->StartX_LEFT);
    RegDebug(L"AnalyzeHidReportDescriptor tp->StartX_RIGHT =", NULL, tp->StartX_RIGHT);

    RegDebug(L"AnalyzeHidReportDescriptor end", NULL, status);
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


void MouseLikeTouchPad_parse(PDEVICE_CONTEXT pDevContext, PTP_REPORT* pPtpReport, mouse_report_t* pMouseReport)
{
    PTP_PARSER* tp = &pDevContext->tp_settings;

    //计算报告频率和时间间隔
    KeQueryTickCount(&tp->current_ticktime);
    tp->ticktime_Interval.QuadPart = (tp->current_ticktime.QuadPart - tp->last_ticktime.QuadPart) * tp->tick_count / 10000;//单位ms毫秒
    tp->TouchPad_ReportInterval = (float)tp->ticktime_Interval.LowPart;//触摸板报告间隔时间ms
    tp->last_ticktime = tp->current_ticktime;


    //保存当前手指坐标
    tp->currentfinger = *pPtpReport;
    UCHAR currentfinger_count = tp->currentfinger.ContactCount;//当前触摸点数量
    UCHAR lastfinger_count=tp->lastfinger.ContactCount; //上次触摸点数量

    BOOLEAN allFingerDetached = TRUE;
    for (UCHAR i = 0; i < MAXFINGER_CNT; i++) {//所有TipSwitch为0时判定为手指全部离开，因为最后一个点离开时ContactCount和Confidence始终为1不会置0。
        if (tp->currentfinger.Contacts[i].TipSwitch) {
            allFingerDetached = FALSE;
            currentfinger_count = tp->currentfinger.ContactCount;//重新定义当前触摸点数量
            break;
        }
    }
    if (allFingerDetached) {
        currentfinger_count = 0;
    }



    //初始化鼠标事件
    pMouseReport->button = 0;
    pMouseReport->dx = 0;
    pMouseReport->dy = 0;
    pMouseReport->h_wheel = 0;
    pMouseReport->v_wheel = 0;

    BOOLEAN Mouse_LButton_Status = 0; //定义临时鼠标左键状态，0为释放，1为按下，每次都需要重置确保后面逻辑
    BOOLEAN Mouse_MButton_Status = 0; //定义临时鼠标中键状态，0为释放，1为按下，每次都需要重置确保后面逻辑
    BOOLEAN Mouse_RButton_Status = 0; //定义临时鼠标右键状态，0为释放，1为按下，每次都需要重置确保后面逻辑

    //初始化当前触摸点索引号，跟踪后未再赋值的表示不存在了
    tp->Mouse_Pointer_CurrentIndexNUM = -1;
    tp->Mouse_LButton_CurrentIndexNUM = -1;
    tp->Mouse_RButton_CurrentIndexNUM = -1;
    tp->Mouse_MButton_CurrentIndexNUM = -1;
    tp->Mouse_Wheel_CurrentIndexNUM = -1;


    //所有手指触摸点的索引号跟踪
    for (char i = 0; i < currentfinger_count; i++) {
        if (tp->Mouse_Pointer_LastIndexNUM != -1) {
            if (tp->lastfinger.Contacts[tp->Mouse_Pointer_LastIndexNUM].ContactID == tp->currentfinger.Contacts[i].ContactID) {
                tp->Mouse_Pointer_CurrentIndexNUM = i;//找到指针
                continue;//查找其他功能
            }
        }

        if (tp->Mouse_Wheel_LastIndexNUM != -1) {
            if (tp->lastfinger.Contacts[tp->Mouse_Wheel_LastIndexNUM].ContactID == tp->currentfinger.Contacts[i].ContactID) {
                tp->Mouse_Wheel_CurrentIndexNUM = i;//找到滚轮辅助键
                continue;//查找其他功能
            }
        }

        if (tp->Mouse_Pointer_LastIndexNUM != -1) {
            if (tp->lastfinger.Contacts[tp->Mouse_Pointer_LastIndexNUM].ContactID == tp->currentfinger.Contacts[i].ContactID) {
                Mouse_LButton_Status = 1; //找到左键，
                tp->Mouse_LButton_CurrentIndexNUM = i;//赋值左键触摸点新索引号
                continue;//查找其他功能
            }
        }

        if (tp->Mouse_RButton_LastIndexNUM != -1) {
            if (tp->lastfinger.Contacts[tp->Mouse_RButton_LastIndexNUM].ContactID == tp->currentfinger.Contacts[i].ContactID) {
                Mouse_RButton_Status = 1; //找到右键，
                tp->Mouse_RButton_CurrentIndexNUM = i;//赋值右键触摸点新索引号
                continue;//查找其他功能
            }
        }

        if (tp->Mouse_MButton_LastIndexNUM != -1) {
            if (tp->lastfinger.Contacts[tp->Mouse_MButton_LastIndexNUM].ContactID == tp->currentfinger.Contacts[i].ContactID) {
                Mouse_MButton_Status = 1; //找到中键，
                tp->Mouse_MButton_CurrentIndexNUM = i;//赋值中键触摸点新索引号
                continue;//查找其他功能
            }
        }     
    }

    if (tp->currentfinger.IsButtonClicked) {//触摸板物理按键功能,切换触控板灵敏度,需要进行离开判定，因为按键报告会一直发送直到释放
        tp->PhysicalButtonUp = FALSE;//准备设置灵敏度
    }
    else {
        if (!tp->PhysicalButtonUp) {
            tp->PhysicalButtonUp = TRUE;
            SetNextSensitivity(pDevContext);//循环设置灵敏度
        }
    }

    //开始鼠标事件逻辑判定
    //注意多手指非同时快速接触触摸板时触摸板报告可能存在一帧中同时新增多个触摸点的情况所以不能用当前只有一个触摸点作为定义指针的判断条件
    if (tp->Mouse_Pointer_LastIndexNUM == -1 && currentfinger_count > 0) {//鼠标指针、左键、右键、中键都未定义,
        //指针触摸点压力、接触面长宽比阈值特征区分判定手掌打字误触和正常操作,压力越小接触面长宽比阈值越大、长度阈值越小
        for (UCHAR i = 0; i < currentfinger_count; i++) {
            if (tp->currentfinger.Contacts[i].ContactID == 0 && tp->currentfinger.Contacts[i].Confidence && tp->currentfinger.Contacts[i].TipSwitch\
                && tp->currentfinger.Contacts[i].Y > tp->StartY_TOP && tp->currentfinger.Contacts[i].X > tp->StartX_LEFT && tp->currentfinger.Contacts[i].X < tp->StartX_RIGHT) {//起点坐标在误触横竖线以内
                tp->Mouse_Pointer_CurrentIndexNUM = i;  //首个触摸点作为指针
                tp->MousePointer_DefineTime = tp->current_ticktime;//定义当前指针起始时间
                break;
            }
        }
    }
    else if (tp->Mouse_Pointer_CurrentIndexNUM == -1 && tp->Mouse_Pointer_LastIndexNUM != -1) {//指针消失
        tp->Mouse_Wheel_mode = FALSE;//结束滚轮模式

        tp->Mouse_Pointer_CurrentIndexNUM = -1;
        tp->Mouse_LButton_CurrentIndexNUM = -1;
        tp->Mouse_RButton_CurrentIndexNUM = -1;
        tp->Mouse_MButton_CurrentIndexNUM = -1;
        tp->Mouse_Wheel_CurrentIndexNUM = -1;
    }
    else if (tp->Mouse_Pointer_CurrentIndexNUM != -1 && !tp->Mouse_Wheel_mode) {  //指针已定义的非滚轮事件处理
        //查找指针左侧或者右侧是否有并拢的手指作为滚轮模式或者按键模式，当指针左侧/右侧的手指按下时间与指针手指定义时间间隔小于设定阈值时判定为鼠标滚轮否则为鼠标按键，这一规则能有效区别按键与滚轮操作,但鼠标按键和滚轮不能一起使用
        //按键定义后会跟踪坐标所以左键和中键不能滑动食指互相切换需要抬起食指后进行改变，左键/中键/右键按下的情况下不能转变为滚轮模式，
        LARGE_INTEGER MouseButton_Interval;
        MouseButton_Interval.QuadPart = (tp->current_ticktime.QuadPart - tp->MousePointer_DefineTime.QuadPart) * tp->tick_count / 10000;//单位ms毫秒
        float Mouse_Button_Interval = (float)MouseButton_Interval.LowPart;//指针左右侧的手指按下时间与指针定义起始时间的间隔ms
        
        if (currentfinger_count > 1) {//触摸点数量超过1才需要判断按键操作
            for (char i = 0; i < currentfinger_count; i++) {
                if (i == tp->Mouse_Pointer_CurrentIndexNUM || i == tp->Mouse_LButton_CurrentIndexNUM || i == tp->Mouse_RButton_CurrentIndexNUM || i == tp->Mouse_MButton_CurrentIndexNUM || i == tp->Mouse_Wheel_CurrentIndexNUM) {//i为正值所以无需检查索引号是否为-1
                    continue;  // 已经定义的跳过
                }
                float dx = (float)(tp->currentfinger.Contacts[i].X - tp->currentfinger.Contacts[tp->Mouse_Pointer_CurrentIndexNUM].X);
                float dy = (float)(tp->currentfinger.Contacts[i].Y - tp->currentfinger.Contacts[tp->Mouse_Pointer_CurrentIndexNUM].Y);
                float distance = sqrt(dx * dx + dy * dy);//触摸点与指针的距离

                if (abs(distance) > tp->FingerMinDistance && abs(distance) < tp->FingerMaxDistance && Mouse_Button_Interval < ButtonPointer_Interval_MSEC) {//指针左右侧有手指按下并且与指针手指起始定义时间间隔小于阈值
                    tp->Mouse_Wheel_mode = TRUE;  //开启滚轮模式
                    tp->Mouse_Wheel_CurrentIndexNUM = i;//滚轮辅助参考手指索引值
                    //手指变化瞬间时电容可能不稳定指针坐标突发性漂移需要忽略
                    tp->JitterFixStartTime = tp->current_ticktime;//抖动修正开始计时
                    tp->Scroll_TotalDistanceX = 0;//累计滚动位移量重置
                    tp->Scroll_TotalDistanceY = 0;//累计滚动位移量重置


                    tp->Mouse_LButton_CurrentIndexNUM = -1;
                    tp->Mouse_RButton_CurrentIndexNUM = -1;
                    tp->Mouse_MButton_CurrentIndexNUM = -1;
                    break;
                }
                else {//前面滚轮模式条件判断已经排除了所以不需要考虑与指针手指起始定义时间间隔，
                    if (tp->Mouse_MButton_CurrentIndexNUM == -1 && abs(distance) > tp->FingerMinDistance && abs(distance) < tp->FingerClosedThresholdDistance && dx < 0) {//指针左侧有并拢的手指按下
                        Mouse_MButton_Status = 1; //找到中键
                        tp->Mouse_MButton_CurrentIndexNUM = i;//赋值中键触摸点新索引号
                        continue;  //继续找其他按键，食指已经被中键占用所以原则上左键已经不可用
                    }
                    else if (tp->Mouse_LButton_CurrentIndexNUM == -1 && abs(distance) > tp->FingerClosedThresholdDistance && abs(distance) < tp->FingerMaxDistance && dx < 0) {//指针左侧有分开的手指按下
                        Mouse_LButton_Status = 1; //找到左键
                        tp->Mouse_LButton_CurrentIndexNUM = i;//赋值左键触摸点新索引号
                        continue;  //继续找其他按键
                    }
                    else if (tp->Mouse_RButton_CurrentIndexNUM == -1 && abs(distance) > tp->FingerMinDistance && abs(distance) < tp->FingerMaxDistance && dx > 0) {//指针右侧有手指按下
                        Mouse_RButton_Status = 1; //找到右键
                        tp->Mouse_RButton_CurrentIndexNUM = i;//赋值右键触摸点新索引号
                        continue;  //继续找其他按键
                    }
                }

            }
        }
        
        //鼠标指针位移设置
        if (currentfinger_count != lastfinger_count) {//手指变化瞬间时电容可能不稳定指针坐标突发性漂移需要忽略
            tp->JitterFixStartTime = tp->current_ticktime;//抖动修正开始计时
        }
        else {
            LARGE_INTEGER FixTimer;
            FixTimer.QuadPart = (tp->current_ticktime.QuadPart - tp->JitterFixStartTime.QuadPart) * tp->tick_count / 10000;//单位ms毫秒
            float JitterFixTimer = (float)FixTimer.LowPart;//当前抖动时间计时

            float STABLE_INTERVAL;
            if (tp->Mouse_MButton_CurrentIndexNUM != -1) {//中键状态下手指并拢的抖动修正值区别处理
                STABLE_INTERVAL = STABLE_INTERVAL_FingerClosed_MSEC;
            }
            else {
                STABLE_INTERVAL = STABLE_INTERVAL_FingerSeparated_MSEC;
            }

            float px = (float)(tp->currentfinger.Contacts[tp->Mouse_Pointer_CurrentIndexNUM].X - tp->lastfinger.Contacts[tp->Mouse_Pointer_LastIndexNUM].X) / tp->thumb_scale;
            float py = (float)(tp->currentfinger.Contacts[tp->Mouse_Pointer_CurrentIndexNUM].Y - tp->lastfinger.Contacts[tp->Mouse_Pointer_LastIndexNUM].Y) / tp->thumb_scale;

            if (JitterFixTimer < STABLE_INTERVAL) {//触摸点稳定前修正
                if (tp->Mouse_LButton_CurrentIndexNUM != -1 || tp->Mouse_RButton_CurrentIndexNUM != -1 || tp->Mouse_MButton_CurrentIndexNUM != -1) {//有按键时修正，单指针时不需要使得指针更精确
                    if (abs(px) <= Jitter_Offset) {//指针轻微抖动修正
                        px = 0;
                    }
                    if (abs(py) <= Jitter_Offset) {//指针轻微抖动修正
                        py = 0;
                    }
                }
            }

            pMouseReport->dx = (UCHAR)(pDevContext->MouseSensitivityValue * px / tp->PointerSensitivity_x);
            pMouseReport->dy = (UCHAR)(pDevContext->MouseSensitivityValue * py / tp->PointerSensitivity_y);

        }
    }
    else if (tp->Mouse_Pointer_CurrentIndexNUM != -1 && tp->Mouse_Wheel_mode) {//滚轮操作模式
        //鼠标指针位移设置
        LARGE_INTEGER FixTimer;
        FixTimer.QuadPart = (tp->current_ticktime.QuadPart - tp->JitterFixStartTime.QuadPart) * tp->tick_count / 10000;//单位ms毫秒
        float JitterFixTimer = (float)FixTimer.LowPart;//当前抖动时间计时

        float px = (float)(tp->currentfinger.Contacts[tp->Mouse_Pointer_CurrentIndexNUM].X - tp->lastfinger.Contacts[tp->Mouse_Pointer_LastIndexNUM].X) / tp->thumb_scale;
        float py = (float)(tp->currentfinger.Contacts[tp->Mouse_Pointer_CurrentIndexNUM].Y - tp->lastfinger.Contacts[tp->Mouse_Pointer_LastIndexNUM].Y) / tp->thumb_scale;

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

        px = (float)(pDevContext->MouseSensitivityValue * px / tp->PointerSensitivity_x);
        py = (float)(pDevContext->MouseSensitivityValue * py / tp->PointerSensitivity_y);

        tp->Scroll_TotalDistanceX += px;//累计滚动位移量
        tp->Scroll_TotalDistanceY += py;//累计滚动位移量

        //判断滚动量
        if (abs(tp->Scroll_TotalDistanceX) > SCROLL_OFFSET_THRESHOLD_X) {//位移量超过阈值
            int h = (int)(abs(tp->Scroll_TotalDistanceX) / SCROLL_OFFSET_THRESHOLD_X);
            pMouseReport->h_wheel = (char)(tp->Scroll_TotalDistanceX > 0 ? h : -h);//滚动行数

            float r = abs(tp->Scroll_TotalDistanceX) - SCROLL_OFFSET_THRESHOLD_X * h;// 滚动位移量余数绝对值
            tp->Scroll_TotalDistanceX = tp->Scroll_TotalDistanceX > 0 ? r : -r;//滚动位移量余数
        }
        if (abs(tp->Scroll_TotalDistanceY) > SCROLL_OFFSET_THRESHOLD_Y) {//位移量超过阈值
            int v = (int)(abs(tp->Scroll_TotalDistanceY) / SCROLL_OFFSET_THRESHOLD_Y);
            pMouseReport->v_wheel = (char)(tp->Scroll_TotalDistanceY > 0 ? v : -v);//滚动行数

            float r = abs(tp->Scroll_TotalDistanceY) - SCROLL_OFFSET_THRESHOLD_Y * v;// 滚动位移量余数绝对值
            tp->Scroll_TotalDistanceY = tp->Scroll_TotalDistanceY > 0 ? r : -r;//滚动位移量余数
        }
    }
    else {
        //其他组合无效
    }

    pMouseReport->button = Mouse_LButton_Status + (Mouse_RButton_Status << 1) + (Mouse_MButton_Status << 2);  //左中右键状态合成

    //保存下一轮所有触摸点的初始坐标及功能定义索引号
    tp->lastfinger = tp->currentfinger;

    lastfinger_count = currentfinger_count;
    tp->Mouse_Pointer_LastIndexNUM = tp->Mouse_Pointer_CurrentIndexNUM;
    tp->Mouse_LButton_LastIndexNUM = tp->Mouse_LButton_CurrentIndexNUM;
    tp->Mouse_RButton_LastIndexNUM = tp->Mouse_RButton_CurrentIndexNUM;
    tp->Mouse_MButton_LastIndexNUM = tp->Mouse_MButton_CurrentIndexNUM;
    tp->Mouse_Wheel_LastIndexNUM = tp->Mouse_Wheel_CurrentIndexNUM;

}



static __forceinline short abs(short x)
{
    if (x < 0)return -x;
    return x;
}

void MouseLikeTouchPad_parse_init(PDEVICE_CONTEXT pDevContext)
{
    PTP_PARSER* tp= &pDevContext->tp_settings;

    tp->Mouse_Pointer_CurrentIndexNUM = -1; //定义当前鼠标指针触摸点坐标的数据索引号，-1为未定义
    tp->Mouse_LButton_CurrentIndexNUM = -1; //定义当前鼠标左键触摸点坐标的数据索引号，-1为未定义
    tp->Mouse_RButton_CurrentIndexNUM = -1; //定义当前鼠标右键触摸点坐标的数据索引号，-1为未定义
    tp->Mouse_MButton_CurrentIndexNUM = -1; //定义当前鼠标中键触摸点坐标的数据索引号，-1为未定义
    tp->Mouse_Wheel_CurrentIndexNUM = -1; //定义当前鼠标滚轮辅助参考手指触摸点坐标的数据索引号，-1为未定义

   tp-> Mouse_Pointer_LastIndexNUM = -1; //定义上次鼠标指针触摸点坐标的数据索引号，-1为未定义
   tp->Mouse_LButton_LastIndexNUM = -1; //定义上次鼠标左键触摸点坐标的数据索引号，-1为未定义
   tp->Mouse_RButton_LastIndexNUM = -1; //定义上次鼠标右键触摸点坐标的数据索引号，-1为未定义
   tp->Mouse_MButton_LastIndexNUM = -1; //定义上次鼠标中键触摸点坐标的数据索引号，-1为未定义
   tp->Mouse_Wheel_LastIndexNUM = -1; //定义上次鼠标滚轮辅助参考手指触摸点坐标的数据索引号，-1为未定义

   tp->Mouse_Wheel_mode = FALSE;

   RtlZeroMemory(&tp->lastfinger, sizeof(PTP_REPORT));
   RtlZeroMemory(&tp->currentfinger, sizeof(PTP_REPORT));

    tp->Scroll_TotalDistanceX = 0;
    tp->Scroll_TotalDistanceY = 0;

    tp->tick_count = KeQueryTimeIncrement();
    KeQueryTickCount(&tp->last_ticktime);

    tp->PhysicalButtonUp = TRUE;

}


void SetNextSensitivity(PDEVICE_CONTEXT pDevContext)
{
    
    UCHAR ms_idx = pDevContext->MouseSensitivityIndex;// MouseSensitivity_Normal;//MouseSensitivity_Slow//MouseSensitivity_FAST

    ms_idx++;
    if (ms_idx == 3) {//灵敏度循环设置
        ms_idx = 0;
    }

    //保存注册表灵敏度设置数值
    NTSTATUS status = SetRegisterMouseSensitivity(pDevContext, ms_idx + 1);//存储的序号值为1/2/3计算更方便
    if (!NT_SUCCESS(status))
    {
        RegDebug(L"SetNextSensitivity SetRegisterMouseSensitivity err", NULL, status);
        return;
    }

    pDevContext->MouseSensitivityIndex = ms_idx;
    pDevContext->MouseSensitivityValue = MouseSensitivityTable[ms_idx];
    RegDebug(L"SetNextSensitivity pDevContext->MouseSensitivityIndex", NULL, pDevContext->MouseSensitivityIndex);

    RegDebug(L"SetNextSensitivity ok", NULL, status);
}


NTSTATUS SetRegisterMouseSensitivity(PDEVICE_CONTEXT pDevContext, ULONG ms_idx)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device = pDevContext->FxDevice;

    DECLARE_CONST_UNICODE_STRING(ValueNameString, L"MouseSensitivityIndex");//效果等同下列多行

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



NTSTATUS GetRegisterMouseSensitivity(PDEVICE_CONTEXT pDevContext, ULONG* ms_idx)
{

    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device = pDevContext->FxDevice;

    WDFKEY hKey = NULL;
    *ms_idx = 0;

    DECLARE_CONST_UNICODE_STRING(ValueNameString, L"MouseSensitivityIndex");//效果等同下列多行

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

    if (hKey) {
        WdfObjectDelete(hKey);
    }

    RegDebug(L"GetRegisterMouseSensitivity end", NULL, status);
    return status;
}
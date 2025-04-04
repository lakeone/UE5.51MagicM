// Copyright Epic Games, Inc. All Rights Reserved.

import { Stack, Text } from '@fluentui/react';
import React from 'react';
import { GetBatchResponse,  GetStepResponse } from '../../../backend/Api';
import { JobDetails } from '../../../backend/JobDetails';
import { getHordeStyling } from '../../../styles/Styles';

export const JobDetailPluginExample: React.FC<{ jobDetails: JobDetails; step?: GetStepResponse; batch?: GetBatchResponse}> = ({ jobDetails, step, batch }) => {

   const { hordeClasses } = getHordeStyling();

    // do not render on batch
    if (batch || !step) {
        return null;
    }

    return <Stack styles={{ root: { paddingTop: 18, paddingRight: 12 } }}>
        <Stack className={hordeClasses.raised}>
            <Stack tokens={{ childrenGap: 12 }}>
                <Text variant="mediumPlus" styles={{ root: { fontFamily: "Horde Open Sans SemiBold" } }}>Job Detail Plugin Example</Text>
                <Text>{`This is an example job detail plugin example, job id is ${jobDetails.id}, step id is ${step.id} `}</Text>
            </Stack>
        </Stack>
    </Stack>;
};
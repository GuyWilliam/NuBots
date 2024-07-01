import { computed, observable } from "mobx";

import { memoize } from "../../base/memoize";
import { AppModel } from "../app/model";
import { RobotModel } from "../robot/model";

export interface DirectorMessage {
    timestamp: Date;
    message: string;
}

export class DirectorModel {
    private appModel: AppModel;

    @observable.ref selectedRobot?: RobotModel;

    constructor(appModel: AppModel) {
        this.appModel = appModel;
    }

    static of = memoize((appModel: AppModel) => {
        return new DirectorModel(appModel);
    });

    @computed
    get robots(): RobotModel[] {
        return this.appModel.robots.filter((robot) => robot.enabled);
    }

    @computed
    get directorRobots(): DirectorRobotModel[] {
        return this.robots.map(DirectorRobotModel.of);
    }

    @computed
    get selectedDirectorRobot(): DirectorRobotModel | undefined {
      return this.selectedRobot ? DirectorRobotModel.of(this.selectedRobot) : undefined;
    }

    // @computed get robots(): DirectorRobotModel[] {
    //     return this.appModel.robots.map((robot) => DirectorRobotModel.of(robot));
    //   }
    // }
}
export interface Provider {
    name: string;
    active: boolean;
    done: boolean;
}

export class DirectorRobotModel {
    robotModel: RobotModel;

    @observable.shallow messages: DirectorMessage[] = [];
    @observable.shallow providers: Provider[] = [];

    // @computed
    // get skills(): string[] {
    //     return this.providers
    //         .filter((provider: Provider) => provider.active)
    //         .map((provider: Provider) => provider.name);
    // }

    constructor(robotModel: RobotModel) {
        this.robotModel = robotModel;
    }

    static of = memoize((robotModel: RobotModel): DirectorRobotModel => {
        return new DirectorRobotModel(robotModel);
    });
}

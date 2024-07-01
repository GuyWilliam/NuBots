import { action } from "mobx";

import { google, message } from "../../../shared/messages";
import { Network } from "../../network/network";
import { NUsightNetwork } from "../../network/nusight_network";
import { RobotModel } from "../robot/model";

import { DirectorRobotModel } from "./model";

import DirectorMessage = message.behaviour.Director;

export class DirectorNetwork {
  constructor(private network: Network) {
    this.network.on(DirectorMessage, this.onDirectorMessage);
  }

  static of(nusightNetwork: NUsightNetwork): DirectorNetwork {
    const network = Network.of(nusightNetwork);
    return new DirectorNetwork(network);
  }

  destroy() {
    this.network.off();
  }

  @action.bound
  private onDirectorMessage(robotModel: RobotModel, message: DirectorMessage) {
    const robot = DirectorRobotModel.of(robotModel);

    robot.providers.clear();
    message.providers.forEach((provider) => {
      const newProvider = {
      layer: provider.name?.match(/message::(.*?)::/)?.[1] ?? "",
      name: provider.name?.match(/message::.*::(.*)/)?.[1] ?? "",
      active: provider.active ?? false,
      done: provider.done ?? false,
      };

      if (!robot.providers.has(newProvider.layer)) {
        robot.providers.set(newProvider.layer, []);
      }
      robot.providers.get(newProvider.layer)?.push(newProvider);


    });
    console.log(robot.providers.keys());
  }
}
